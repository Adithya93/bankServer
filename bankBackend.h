#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "./bankDBHandler.h"

class bankBackend {
    
    public:

        // constructor, initializes data structures
        bankBackend();

        // for balance queries
        float getBalance(unsigned long account);

        // for creating/resetting accounts and transfers
        //bool setBalance(unsigned long account, float balance, bool reset);
        bool setBalance(unsigned long account, float balance, bool reset, bool writtenToDB = false);

        // for transfers
        //bool saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags);
        int saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags);

        // for transfers
        //bool transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags);
        int transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags);

        std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> query(std::string queryString); 

        std::vector<std::tuple<bool, std::string>> createAccounts(std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqs);
     
        std::vector<std::tuple<float, std::string>> getBalances(std::vector<std::tuple<unsigned long, std::string>>  balanceReqs);

        //std::vector<std::tuple<bool, std::string>> doTransfers(std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs);
        std::vector<std::tuple<int, std::string>> doTransfers(std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs);

        std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> doQueries(std::vector<std::tuple<std::string, std::string>> queryReqs); 

        void cleanUp();

    private:
        // helper method
        std::tuple<std::map<unsigned long, float>::iterator, bool> findAccount(unsigned long account);
        std::map<unsigned long, float>* cache; // KIV : Eviction Policy/LRU ? Can use deque to track LRU account / LRU transfer ptr
        //std::unordered_set<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>*>* transactions;
        bankDBHandler* dbHandler;
        std::mutex  cacheMutex;


}; 