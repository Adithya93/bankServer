#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <iostream>
#include <vector>

using namespace xercesc;

class bankRequestParser {
    public:
        XercesDOMParser* parser;
        MemBufInputSource* source;
        ErrorHandler* errHandler;

        bankRequestParser(const char* requestBuffer, size_t requestSize);

        int parseRequest();

        int testValidity();

        int parseCreateReqs(); 

        int parseBalanceReqs();

        int parseTransferReqs();

        std::vector<std::tuple<unsigned long, float, bool, std::string>> getCreateReqs();

        std::vector<std::tuple<unsigned long, std::string>> getBalanceReqs();

        //std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<string>*>*>* getTransferReqs(); // can use instead of below line if txn cache is activated
        std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> getTransferReqs();

        bool hasReset();

        void cleanUp();


    private:
      std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqs;
      std::vector<std::tuple<unsigned long, std::string>>  balanceReqs;
      std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs;
      bool reset;
      DOMElement* root;

};
