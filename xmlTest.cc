#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>

#include <iostream>

#include <map>
#include <unordered_set>
#include <vector>
#include <queue>

using namespace std;
using namespace xercesc;


map<unsigned long, float>* cache; // KIV : Eviction Policy/LRU ? Can use deque to track LRU account / LRU transfer ptr
unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*>* transactions;


map<unsigned long, float>::iterator findAccount(unsigned long account) {
    return cache->find(account);
}


// for balance queries
float getBalance(unsigned long account) {
    map<unsigned long, float>::iterator it = findAccount(account);//cache->find(account);
    if (it != cache->end()) {
        float foundBalance = it->second;
        printf("Current balance of account %lu : %f\n", account, foundBalance);
        return foundBalance;
    }
    printf("Account %lu not in cache\n", account);
    return -1;
}

// for creating/resetting accounts and transfers
bool setBalance(unsigned long account, float balance, bool reset) {
    map<unsigned long, float>::iterator it = findAccount(account);
    if (it == cache->end()) { // creating new account
        pair<map<unsigned long, float>::iterator, bool> inserted = cache->insert(pair<unsigned long, float>(account, balance));
        printf("Created new account %lu with balance %f\n", inserted.first->first, inserted.first->second);
        return true;
    }
    else if (!reset) { // account already exists and not a reset, do not update
        printf("Account %lu already exists, ignoring\n", account);
        return false;
    }
    else {     // updating existing account's balance
        it->second = balance;
        printf("Overwrote account %lu's balance to %f\n", it->first, it->second);
        return true;
    }
}

// for transfers
void saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, vector<string>* tags) {
    tuple<unsigned long, unsigned long, float, vector<string>*>* transTup = new tuple<unsigned long, unsigned long, float, vector<string>*>(fromAccount, toAccount, amount, tags);
    transactions->insert(transTup);
    return;
}

// for transfers
bool transfer(unsigned long fromAccount, unsigned long toAccount, float amount, vector<string>* tags) {
    unsigned long from;
    unsigned long to;
    if (amount > 0) {
        from = fromAccount;
        to = toAccount;
    }
    else if (amount < 0) {
        from = toAccount;
        to = fromAccount;
        amount *= -1;
    }
    else {
        printf("Ignoring 0 transfer from %lu to %lu\n", fromAccount, toAccount);
        return false;
    }
    map<unsigned long, float>::iterator fromIt = findAccount(from);
    map<unsigned long, float>::iterator toIt = findAccount(to);
    if (fromIt == cache->end() || toIt == cache->end()) {
        printf("Non-existent account in transfer from %lu to %lu, ignoring\n", fromAccount, toAccount);
        return false;
    }
    if (fromIt->second < amount) {
        printf("Insufficient funds in account %lu, unable to transfer %f\n", from, amount);
        return false;
    }
    setBalance(from, fromIt->second - amount, true);
    setBalance(to, toIt->second + amount, true);
    saveTransfer(fromAccount, toAccount, amount, tags);
    return true;
}

// for queries
unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> processORQuery(unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> querySet, vector<>) {

    return NULL;
}


unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> processQueryAmountConds() {

    return NULL;
}




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

    cache = new map<unsigned long, float>();
    transactions = new unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*>();
    
    getBalance(1);
    setBalance(1, 5, false);
    getBalance(1);
    setBalance(1, 10, true);
    getBalance(1);
    setBalance(2, 20, false);
    setBalance(2, 30, false);
    getBalance(2);
    transfer(3, 1, 5, NULL);
    transfer(1, 3, 5, NULL);
    transfer(1, 2, 15, NULL);
    transfer(2, 1, -15, NULL);
    transfer(1, 2, 0, NULL);
    transfer(1, 2, 5, NULL);
    getBalance(1);
    getBalance(2);

    XercesDOMParser* parser = new XercesDOMParser();
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    parser->setDoNamespaces(true);    // optional

    ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
    parser->setErrorHandler(errHandler);

    const char* xmlFile = "test.xml";

    try {
        parser->parse(xmlFile);


        // Try to extract info
        DOMDocument* xmlDoc = parser->getDocument();
        if (!xmlDoc) {
            //delete parser;
            cout << "Unable to open document!\n";
        }
        else {
            DOMElement* root = xmlDoc->getDocumentElement();
            if (!root) {
                cout << "Empty document!\n";
            }
            //wcout << "Root tag name: " << (char *)(root->getTagName()) << "\n";
            const XMLCh* rootTagName = root->getTagName();
            wprintf(L"Root tag name: %s\n", XMLString::transcode(rootTagName));
            const XMLCh* rootAttrName = XMLString::transcode("reset");
            DOMAttr* rootAttr = root->getAttributeNode(rootAttrName);
            const XMLCh* rootName = rootAttr->getName();
            const XMLCh* rootValue = rootAttr->getValue();
            wprintf(L"Attribute name: %s; Attribute value: %s\n", XMLString::transcode(rootName), XMLString::transcode(rootValue));
        
            DOMNodeList* creations = root->getElementsByTagName(XMLString::transcode("create"));
            printf("No. of creations: %lu\n", creations->getLength());
            //iterate through DOMNodes
            for (int i = 0; i < creations->getLength(); i ++) {
                //DOMNodeList* children = creations->item(i)->getOwnerDocument()->getDocumentElement()->getElementsByTagName(XMLString::transcode("account"));
                DOMNode* child = creations->item(i);
                const XMLCh* nodeText = child->getTextContent();
                wprintf(L"Node value: %s\n", nodeText);
                const XMLCh* accountVal = child->getAttributes()->getNamedItem(XMLString::transcode("ref"))->getTextContent();
                wprintf(L"Ref: %s\n", XMLString::transcode(accountVal));
                DOMNodeList* grandchildren = child->getChildNodes();
                for (int j = 0; j < grandchildren->getLength(); j ++) {
                    DOMNode* infoNode = grandchildren->item(j);
                    const char * nodeName = XMLString::transcode(infoNode->getNodeName());
                    if (strcmp(nodeName, "account") == 0) {
                        printf("Account Found. %s : %s\n", nodeName, XMLString::transcode(infoNode->getTextContent()));
                    }
                    else if (strcmp(nodeName, "balance") == 0) {
                        printf("Balance Found. %s : %s\n", nodeName, XMLString::transcode(infoNode->getTextContent()));
                    } 

                    //wprintf(L"Grandchild name: %s\n", XMLString::transcode(infoNode->getNodeName()));
                    //wprintf(L"Grandchild value: %s\n", XMLString::transcode(infoNode->getTextContent()));
                    //wprintf()
                }

                /*
                for (int j = 0; j < children->getLength(); j ++) {
                    const XMLCh* accountVal = children->item(j)->getTextContent();
                    wprintf(L"Account value: %s\n", XMLString::transcode(accountVal));
                }
                */
                printf("Done with creation %d\n", i);
            }
            //~creations();
            //delete creations;
            //wcout << "Length of root tag name: " << strlen((char*)(root->getTagName())) << "\n";
            //DOMAttr* rootAttr = root->getAttributeNode("reset");
            //XMLCh* rootName = rootAttr->getName();
            //XMLCh* rootValue = rootAttr->getValue();
            //wcout << "Attribute name: " << rootName << "; Attribute value: " << rootValue;
        }

    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.msg);
        cout << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "Unexpected Exception \n" ;
        return -1;
    }

    delete cache;
    delete parser;
    delete errHandler;
    
    return 0;
}
          