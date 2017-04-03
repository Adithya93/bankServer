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

using namespace std;
using namespace xercesc;

class bankRequestParser {
    public:
        XercesDOMParser* parser;
        MemBufInputSource* source;
        ErrorHandler* errHandler;

        bankRequestParser(const char* requestBuffer, size_t requestSize) { // TO-DO : Refactor for better exception safety?
            parser = (XercesDOMParser*) new XercesDOMParser();
            parser->setValidationScheme(XercesDOMParser::Val_Always);
            parser->setDoNamespaces(true);    // optional
            errHandler = (ErrorHandler*) new HandlerBase();
            parser->setErrorHandler(errHandler);
            source = (MemBufInputSource*) new MemBufInputSource((const XMLByte*)requestBuffer, (const XMLSize_t) requestSize, "test"); // TO-DO: Use threadId or some identifier as last arg
        }

        int parseRequest() { 
            try {
                //xmlDoc = parser->parse(*source);
                parser->parse(*source);
                puts("Successfully parsed!");
                if (testValidity()) {
                  puts("Invalid XML document, exiting");
                  cleanUp();
                  return -1;
                }
                if (parseCreateReqs()) cout << "Unable to parse create requests";
                if (parseBalanceReqs()) cout << "Unable to parse balance requests";
                if (parseTransferReqs()) cout << "Unable to parse transfer requests";
                return 0;
            }

            catch (const XMLException& toCatch) {
                char* message = XMLString::transcode(toCatch.getMessage());
                cout << "Exception message is: \n"
                     << message << "\n";
                XMLString::release(&message);
                cleanUp(); 
                return -1;
            }
            catch (const DOMException& toCatch) {
                char* message = XMLString::transcode(toCatch.msg);
                cout << "Exception message is: \n"
                     << message << "\n";
                XMLString::release(&message);
                cleanUp();
                return -1;
            }
            catch (...) {
                cout << "Unexpected Exception \n" ;
                cleanUp();
                return -1;
            }

        }

        int testValidity() {
          DOMDocument* xmlDoc = parser->getDocument();
          if (!xmlDoc) {
            cout << "Unable to open document!\n";
            return -1;
          }
          else {
            root = xmlDoc->getDocumentElement();
            if (!root) {
              cout << "Empty document!\n";
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

        int parseCreateReqs() {
          DOMNodeList* creations = root->getElementsByTagName(XMLString::transcode("create"));
          printf("No. of creations: %lu\n", creations->getLength());
          //iterate through DOMNodes
          vector<tuple<unsigned long, float, bool, string>*>* createReqsVec = new vector<tuple<unsigned long, float, bool, string>*>(); // exception-safety? :(
          //vector<tuple<unsigned long, float, bool, string>> createReqsVec;
          tuple<unsigned long, float, bool, string>* createReq;
          for (int i = 0; i < creations->getLength(); i ++) {

              unsigned long account = 0;
              float balance = 0;
              string ref;

              DOMNode* child = creations->item(i);
              const XMLCh* nodeText = child->getTextContent();
              wprintf(L"Node value: %s\n", nodeText);
              const XMLCh* refXML = child->getAttributes()->getNamedItem(XMLString::transcode("ref"))->getTextContent();
              ref = XMLString::transcode(refXML);
              cout << "Ref: " << ref << "\n";
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
                        balance = stof(nodeValue);
                      }
                      catch (const invalid_argument& ia) {
                        cerr << "Invalid formatting of balance in create request: " << ia.what() << "\n";
                        // continue processing other creates in this request instead of returning error
                      }
                  } 
              }
              createReqsVec->push_back(new tuple<unsigned long, float, bool, string>(account, balance, reset, ref));
              printf("Done with creation %d\n", i);
          }
          createReqs = createReqsVec;
          return 0;
        }


        int parseBalanceReqs() {
          return 0;
        }

        int parseTransferReqs() {
          return 0;
        }

        vector<tuple<unsigned long, float, bool, string>*>* getCreateReqs() {
          return createReqs;
        }

        vector<tuple<unsigned long, string>*>* getBalanceReqs() {
          return balanceReqs;
        }

        vector<tuple<unsigned long, unsigned long, float, string>*>* getTransferReqs() {
          return transferReqs;
        }

        bool hasReset() {
          return reset;
        }

        void cleanUp() {
            delete parser;
            delete errHandler;
            delete source;
        }


    private:
      vector<tuple<unsigned long, float, bool, string>*>* createReqs;
      vector<tuple<unsigned long, string>*>*  balanceReqs;
      vector<tuple<unsigned long, unsigned long, float, string>*>* transferReqs;
      bool reset;
      DOMElement* root;

};





int main (int argc, char* args[]) {
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Error during initialization! :\n"
             << message << "\n";
        XMLString::release(&message);
        return 1;
    }

    const char* xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<transactions reset=\"true\">\
  <create ref=\"c1\">\
    <account>1234</account>\
    <balance>500</balance>\
  </create>\
  <create ref=\"c2\">\
    <account>5678</account>\
  </create>\
    <create ref=\"c3\">\
    <account>1000</account>\
    <balance>500000</balance>\
  </create>\
  <create ref=\"c4\">\
    <account>1001</account>\
    <balance>5000000</balance>\
  </create>\
  <transfer ref=\"1\">\
    <to>1234</to>\
    <from>1000</from>\
    <amount>9568.34</amount>\
    <tag>paycheck</tag>\
    <tag>monthly</tag>\
  </transfer>\
  <transfer ref=\"2\">\
    <from>1234</from>\
    <to>1001</to>\
    <amount>100.34</amount>\
    <tag>food</tag>\
  </transfer>\
  <transfer ref=\"3\">\
    <from>1234</from>\
    <to>5678</to>\
    <amount>345.67</amount>\
    <tag>saving</tag>\
  </transfer>\
  <balance ref=\"xyz\">\
    <account>1234</account>\
  </balance>\
  <query ref=\"4\">\
    <or>\
      <equals from=\"1234\"/>\
      <equals to=\"5678\"/>\
</or>\
    <greater amount=\"100\"/>\
  </query>\
</transactions>\0";
    puts("About to allocate bankRequestParser");
    bankRequestParser* test = new bankRequestParser(xmlString, strlen(xmlString));    
    test->parseRequest();
    vector<tuple<unsigned long, float, bool, string>*>* testCreates = test->getCreateReqs();
    for (vector<tuple<unsigned long, float, bool, string>*>::iterator it = testCreates->begin(); it < testCreates->end(); it ++ ) {
      tuple<unsigned long, float, bool, string> testCreate = **it;
      cout << "Account: " << get<0>(testCreate) << "; " << get<1>(testCreate) << "; " << get<2>(testCreate) << ";" << get<3>(testCreate) << "\n";
    }
    test->cleanUp();
    delete test;
    XMLPlatformUtils::Terminate();
    return 0;
}
          