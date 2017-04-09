#include "./bankRequestParser.h"

using namespace xercesc;

  bankRequestParser::bankRequestParser() { // TO-DO : Refactor for better exception safety?
    //initialize();      
  }


  void bankRequestParser::initialize(const char* requestBuffer, size_t requestSize) {
    parser = (XercesDOMParser*) new XercesDOMParser();
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);    // optional
    errHandler = (ErrorHandler*) new HandlerBase();
    parser->setErrorHandler(errHandler);
    source = (MemBufInputSource*) new MemBufInputSource((const XMLByte*)requestBuffer, (const XMLSize_t) requestSize, "test"); // TO-DO: Use threadId or some identifier as last arg  
  }

  void bankRequestParser::cleanUp() {
      delete parser;
      delete errHandler;
      delete source;
  }


  /*
  // Called after servicing every request
  void bankRequestParser::clearRequest() {
    cleanUp();
    //initialize();
  }
  */

  int bankRequestParser::parseRequest() { 
      try {
          parser->parse(*source);
          puts("Successfully parsed!");
          if (testValidity()) {
            puts("Invalid XML document, exiting");
            cleanUp();
            return -1;
          }
          if (parseCreateReqs()) std::cout << "Unable to parse create requests\n";
          if (parseBalanceReqs()) std::cout << "Unable to parse balance requests\n";
          if (parseTransferReqs()) std::cout << "Unable to parse transfer requests\n";
          if (parseQueryReqs()) std::cout<< "Error while parsing query requests\n";
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
        //std::cout << "Ref: " << ref << "\n";
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
          //std::cout << "Skipping non top-level balance tag\n";
          continue;
        }
        const XMLCh* refXML = refAttrNode->getTextContent();
        ref = XMLString::transcode(refXML);
        //std::cout << "Ref: " << ref << "\n";
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
    DOMNodeList* transfers = root->getElementsByTagName(XMLString::transcode("transfer"));
    printf("No. of transfers: %lu\n", transfers->getLength());
    //iterate through DOMNodes
    //std::vector<std::tuple<unsigned long, float, bool, std::string>*>* createReqsVec;// = new std::vector<std::tuple<unsigned long, float, bool, std::string>*>(); // exception-safety? :(
    std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqsVec;
    //std::tuple<unsigned long, float, bool, std::string> createReq;
    for (int i = 0; i < transfers->getLength(); i ++) {

        unsigned long fromAccount = 0;
        unsigned long toAccount = 0;
        float amount = 0;
        std::string ref;
        std::vector<std::string> tags;

        DOMNode* child = transfers->item(i);
        const XMLCh* nodeText = child->getTextContent();
        wprintf(L"Node value: %s\n", nodeText);
        const XMLCh* refXML = child->getAttributes()->getNamedItem(XMLString::transcode("ref"))->getTextContent();
        ref = XMLString::transcode(refXML);
        //std::cout << "Ref: " << ref << "\n";
        DOMNodeList* grandchildren = child->getChildNodes();
        for (int j = 0; j < grandchildren->getLength(); j ++) {
            DOMNode* infoNode = grandchildren->item(j);
            const char * nodeName = XMLString::transcode(infoNode->getNodeName());
            const char * nodeValue = XMLString::transcode(infoNode->getTextContent());
            if (strcmp(nodeName, "from") == 0) {
                printf("fromAccount Found. %s : %s\n", nodeName, nodeValue);
                if (!(fromAccount = strtoul(nodeValue, NULL, 10))) { // strtoul returns 0, invalid formatting of account num
                  printf("Invalid formatting of account number in transfer request: %s\n", nodeValue);
                  // continue processing other creates in this request instead of returning error
                }
            }
            else if (strcmp(nodeName, "to") == 0) {
                printf("toAccount Found. %s : %s\n", nodeName, nodeValue);
                if (!(toAccount = strtoul(nodeValue, NULL, 10))) { // strtoul returns 0, invalid formatting of account num
                  printf("Invalid formatting of account number in transfer request: %s\n", nodeValue);
                  // continue processing other creates in this request instead of returning error
                }
            }
            else if (strcmp(nodeName, "amount") == 0) {
                printf("Amount Found. %s : %s\n", nodeName, nodeValue);
                try {
                  amount = std::stof(nodeValue);
                }
                catch (const std::invalid_argument& ia) {
                  std::cerr << "Invalid formatting of balance in transfer request: " << ia.what() << "\n";
                  // continue processing other creates in this request instead of returning error
                }
            }
            else if (strcmp(nodeName, "tag") == 0) {
              printf("Tag found. %s : %s\n", nodeName, nodeValue);
              tags.push_back(std::string(nodeValue));
            } 
        }
        transferReqsVec.push_back(std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>(fromAccount, toAccount, amount, ref, tags));
        printf("Done with transfer %d\n", i);
    }
    transferReqs = transferReqsVec;
    return 0;
  }


  int bankRequestParser::parseQueryReqs() {
    DOMNodeList* queries = root->getElementsByTagName(XMLString::transcode("query"));
    printf("No. of queries: %lu\n", queries->getLength());
    //iterate through DOMNodes
    std::vector<std::tuple<std::string, std::string>> queryReqsVec;
    int hasError = 0;
    for (int i = 0; i < queries->getLength(); i ++) {
      std::string ref;
      DOMNode* child = queries->item(i);
      //const XMLCh* nodeText = child->getTextContent();
      //wprintf(L"Node value: %s\n", nodeText);
      const XMLCh* refXML = child->getAttributes()->getNamedItem(XMLString::transcode("ref"))->getTextContent();
      ref = XMLString::transcode(refXML);
      //std::cout << "Ref: " << ref << "\n";
      DOMNodeList* grandchildren = child->getChildNodes();

      //bool implictAnd = grandchildren->getLength() > 1; // if more than 1 tag directly under <query>, they are implicitly ANDed together
      queryNode* root;
      if (grandchildren->getLength() > 1) { // implicit AND
        //std::cout << "No. of top-level children of query: " << grandchildren->getLength() << "; implicit AND detected\n";
        root = new queryNode('a');
        for (int childNodeNum = 0; childNodeNum < grandchildren->getLength(); childNodeNum ++) {
          queryNode* rootChild = parseQueryNode(grandchildren->item(childNodeNum));
          if (!rootChild) {
            deleteQueryNodes(root);
            root = NULL;
            hasError = 1;
            break;
          }
          root->addChild(rootChild);
        }
      }

      else if (grandchildren->getLength() == 0) { // empty query
        //std::cout << "Empty query tag, treating as TRUE\n";
        queryReqsVec.push_back(std::tuple<std::string, std::string>("TRUE", ref));
        printf("Done with query %d\n", i);
        continue;
      }
      
      else { // get tag name of top-level tag within query
        root = parseQueryNode(grandchildren->item(0));
      }
      std::string queryString;
      if (!root) {
        std::cout << "Query " << i << " of ref " << ref << " is invalid\n";
        queryString = "";
        hasError = 1;
      }
      else {
        queryString = root->getQueryString();
        //std::cout << "Query String for query number " << i << " of ref " << ref << " is " << queryString << "\n";
        deleteQueryNodes(root);
      }
      queryReqsVec.push_back(std::tuple<std::string, std::string>(queryString, ref));      
      printf("Done with query %d\n", i);
    }
    queryReqs = queryReqsVec;
    return hasError;
  }


  queryNode* bankRequestParser::parseQueryNode(DOMNode* domNode) {
      queryNode* q;
      const char * nodeName = XMLString::transcode(domNode->getNodeName());
      //std::cout << "Node name: " << nodeName << "\n";
      char rel;
      char attr;
      if (strcmp(nodeName, "and") == 0) {
        rel = 'a';
      }
      else if (strcmp(nodeName, "or") == 0) {
        rel = 'o';
      }
      else if (strcmp(nodeName, "not") == 0) {
        rel = 'n';
      }
      else { // relational operator - Base-Case : No child nodes
        
        DOMNamedNodeMap * attrs = domNode->getAttributes();
        if (attrs->getLength() != 1) {
          std::cout << "Wrong number of attributes in query relational operator: " << attrs->getLength() << "\n";
          // can flag this query as invalid and move on to next query
          return NULL;
        }
        DOMNode* attrNode = attrs->item(0);
        const char * attrNodeName = XMLString::transcode(attrNode->getNodeName());
        const char * attrNodeValue = XMLString::transcode(attrNode->getNodeValue());
        //std::cout << "Attribute value: " << attrNodeValue << "\n";
        std::string val(attrNodeValue);
        if (strcmp(attrNodeName, "from") == 0) {
          attr = 'f';
        }
        else if (strcmp(attrNodeName, "to") == 0) {
          attr = 't';
        }
        else if (strcmp(attrNodeName, "amount") == 0) {
          attr = 'a';
        }
        else { // invalid attribute
          std::cout << "Unknown transaction attribute specified in query relational operator: " << attrNodeName << "\n";
          // can flag this query as invalid and move on to next query
          return NULL;
        }
        if (strcmp(nodeName, "greater") == 0) {
          rel = '>';
        }
        else if (strcmp(nodeName, "less") == 0) {
          rel = '<';
        }
        else if (strcmp(nodeName, "equals") == 0) {
          rel = '=';
        }
        else { // invalid operator
          std::cout << "Invalid relational operator\n";
          return NULL;
        }
        q = new queryNode(rel, val, attr);
        //std::cout << "Adding simple node with query " << q->getQueryString() << "\n";
        return q;
      }
      // logical operator: add children, and if any child invalid, invalidate entire query tree
      q = new queryNode(rel);
      DOMNodeList* nodeChildren = domNode->getChildNodes();
      for (int childNum = 0; childNum < nodeChildren->getLength(); childNum ++) {
        queryNode * childQueryNode = parseQueryNode(nodeChildren->item(childNum));
        if (!childQueryNode) {
          // free all resources and return NULL
          deleteQueryNodes(q); // q freed
          return NULL;
        }
        q->addChild(childQueryNode);
      }
      return q;
  }


  void bankRequestParser::deleteQueryNodes(queryNode * q) {
    /*
    std::vector<queryNode*>* qChildren = q->getChildren();
    for (std::vector<queryNode*>::iterator it = qChildren->begin(); it < qChildren->end(); it ++) {
      deleteQueryNodes(*it);
    }
    q->deleteNode(); // Free memory allocated by q
    */
    q->deleteNode();
    delete q;
  }

  /*
  void deleteRootNodeChildren(queryNode * root) {
    std::vector<queryNode*>* rootChildren = root->getChildren();
    for (std::vector<queryNode*>::iterator it = rootChildren->begin(); it < rootChildren->end(); it ++) {
      deleteQueryNodes(*it);
    }
  }
  */

  std::vector<std::tuple<unsigned long, float, bool, std::string>> bankRequestParser::getCreateReqs() {
    return createReqs;
  }

  std::vector<std::tuple<unsigned long, std::string>> bankRequestParser::getBalanceReqs() {
    return balanceReqs;
  }

  //std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<string>*>*>* bankRequestParser::getTransferReqs() {
  std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> bankRequestParser::getTransferReqs() {
    return transferReqs;
  }

  std::vector<std::tuple<std::string, std::string>> bankRequestParser::getQueryReqs() {
    return queryReqs;
  }

  bool bankRequestParser::hasReset() {
    return reset;
  }

  