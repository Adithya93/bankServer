#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <queue>
#include "./bankRequestParser.h"

using namespace xercesc;
  bankRequestParser::bankRequestParser(const char* requestBuffer, size_t requestSize) { // TO-DO : Refactor for better exception safety?
      parser = (XercesDOMParser*) new XercesDOMParser();
      parser->setValidationScheme(XercesDOMParser::Val_Always);
      parser->setDoNamespaces(true);    // optional
      errHandler = (ErrorHandler*) new HandlerBase();
      parser->setErrorHandler(errHandler);
      source = (MemBufInputSource*) new MemBufInputSource((const XMLByte*)requestBuffer, (const XMLSize_t) requestSize, "test"); // TO-DO: Use threadId or some identifier as last arg
  }

  int bankRequestParser::parseRequest() { 
      try {
          parser->parse(*source);
          puts("Successfully parsed!");
          if (testValidity()) {
            puts("Invalid XML document, exiting");
            cleanUp();
            return -1;
          }
          if (parseCreateReqs()) std::cout << "Unable to parse create requests";
          if (parseBalanceReqs()) std::cout << "Unable to parse balance requests";
          if (parseTransferReqs()) std::cout << "Unable to parse transfer requests";
          return 0;
      }

      catch (const XMLException& toCatch) {
          char* message = XMLString::transcode(toCatch.getMessage());
          std::cout << "Exception message is: \n" << message << "\n";
          XMLString::release(&message);
          cleanUp(); 
          return -1;
      }
      catch (const DOMException& toCatch) {
          char* message = XMLString::transcode(toCatch.msg);
          std::cout << "Exception message is: \n" << message << "\n";
          XMLString::release(&message);
          cleanUp();
          return -1;
      }
      catch (...) {
          std::cout << "Unexpected Exception \n" ;
          cleanUp();
          return -1;
      }

  }

  int bankRequestParser::testValidity() {
    DOMDocument* xmlDoc = parser->getDocument();
    if (!xmlDoc) {
      std::cout << "Unable to open document!\n";
      return -1;
    }
    else {
      root = xmlDoc->getDocumentElement();
      if (!root) {
        std::cout << "Empty document!\n";
        return -1;
      }
      const XMLCh* rootTagName = root->getTagName();
      const char * rootTagNameChar = XMLString::transcode(rootTagName);
      printf("Root tag name: %s\n", rootTagNameChar);
      if (strcmp(rootTagNameChar, "transactions") != 0) {
        printf("Invalid top-level tag of %s\n", rootTagNameChar);
        return -1;
      }
      const XMLCh* rootAttrName = XMLString::transcode("reset");
      DOMAttr* rootAttr = root->getAttributeNode(rootAttrName);
      const XMLCh* rootName = rootAttr->getName();
      const XMLCh* rootValue = rootAttr->getValue();
      const char* rootValueChar = XMLString::transcode(rootValue);
      //wprintf(L"Attribute name: %s; Attribute value: %s\n", , XMLString::transcode(rootValue));
      printf("Reset attribute value: %s\n", rootValueChar);
      if (strcmp(rootValueChar, "true") == 0) {
        reset = true;
      }
      else {
        reset = false;
      }
      return 0;
    }
  }

  int bankRequestParser::parseCreateReqs() {
    DOMNodeList* creations = root->getElementsByTagName(XMLString::transcode("create"));
    printf("No. of creations: %lu\n", creations->getLength());
    //iterate through DOMNodes
    //std::vector<std::tuple<unsigned long, float, bool, std::string>*>* createReqsVec;// = new std::vector<std::tuple<unsigned long, float, bool, std::string>*>(); // exception-safety? :(
    std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqsVec;
    //std::tuple<unsigned long, float, bool, std::string> createReq;
    for (int i = 0; i < creations->getLength(); i ++) {

        unsigned long account = 0;
        float balance = 0;
        std::string ref;

        DOMNode* child = creations->item(i);
        const XMLCh* nodeText = child->getTextContent();
        wprintf(L"Node value: %s\n", nodeText);
        const XMLCh* refXML = child->getAttributes()->getNamedItem(XMLString::transcode("ref"))->getTextContent();
        ref = XMLString::transcode(refXML);
        std::cout << "Ref: " << ref << "\n";
        DOMNodeList* grandchildren = child->getChildNodes();
        for (int j = 0; j < grandchildren->getLength(); j ++) {
            DOMNode* infoNode = grandchildren->item(j);
            const char * nodeName = XMLString::transcode(infoNode->getNodeName());
            const char * nodeValue = XMLString::transcode(infoNode->getTextContent());
            if (strcmp(nodeName, "account") == 0) {
                printf("Account Found. %s : %s\n", nodeName, nodeValue);
                if (!(account = strtoul(nodeValue, NULL, 10))) { // strtoul returns 0, invalid formatting of account num
                  printf("Invalid formatting of account number in create request: %s\n", nodeValue);
                  // continue processing other creates in this request instead of returning error
                }
            }
            else if (strcmp(nodeName, "balance") == 0) {
                printf("Balance Found. %s : %s\n", nodeName, nodeValue);
                try {
                  balance = std::stof(nodeValue);
                }
                catch (const std::invalid_argument& ia) {
                  std::cerr << "Invalid formatting of balance in create request: " << ia.what() << "\n";
                  // continue processing other creates in this request instead of returning error
                }
            } 
        }
        createReqsVec.push_back(std::tuple<unsigned long, float, bool, std::string>(account, balance, reset, ref));
        printf("Done with creation %d\n", i);
    }
    createReqs = createReqsVec;
    return 0;
  }


  int bankRequestParser::parseBalanceReqs() {
    DOMNodeList* balances = root->getElementsByTagName(XMLString::transcode("balance"));
    printf("No. of balances: %lu\n", balances->getLength());
    std::vector<std::tuple<unsigned long, std::string>> balanceReqsVec;
    for (int i = 0; i < balances->getLength(); i ++) {
        unsigned long account = 0;
        std::string ref;

        DOMNode* child = balances->item(i);
        DOMNode* refAttrNode;
        if (!(refAttrNode = child->getAttributes()->getNamedItem(XMLString::transcode("ref")))) {
          std::cout << "Skipping non top-level balance tag\n";
          continue;
        }
        const XMLCh* refXML = refAttrNode->getTextContent();
        ref = XMLString::transcode(refXML);
        std::cout << "Ref: " << ref << "\n";
        DOMNodeList* grandchildren = child->getChildNodes();
        for (int j = 0; j < grandchildren->getLength(); j ++) {
            DOMNode* infoNode = grandchildren->item(j);
            const char * nodeName = XMLString::transcode(infoNode->getNodeName());
            const char * nodeValue = XMLString::transcode(infoNode->getTextContent());
            if (strcmp(nodeName, "account") == 0) {
                printf("Account Found. %s : %s\n", nodeName, nodeValue);
                if (!(account = strtoul(nodeValue, NULL, 10))) { // strtoul returns 0, invalid formatting of account num
                  printf("Invalid formatting of account number in create request: %s\n", nodeValue);
                  // continue processing other creates in this request instead of returning error
                }
            }
        }
        balanceReqsVec.push_back(std::tuple<unsigned long, std::string>(account, ref));
        printf("Done with creation %d\n", i);
    }
    balanceReqs = balanceReqsVec;
    return 0;
  }

  int bankRequestParser::parseTransferReqs() {
    return 0;
  }

  std::vector<std::tuple<unsigned long, float, bool, std::string>> bankRequestParser::getCreateReqs() {
    return createReqs;
  }

  std::vector<std::tuple<unsigned long, std::string>> bankRequestParser::getBalanceReqs() {
    return balanceReqs;
  }

  std::vector<std::tuple<unsigned long, unsigned long, float, std::string>> bankRequestParser::getTransferReqs() {
    return transferReqs;
  }

  bool bankRequestParser::hasReset() {
    return reset;
  }

  void bankRequestParser::cleanUp() {
      delete parser;
      delete errHandler;
      delete source;
  }