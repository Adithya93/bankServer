#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <queue>
#include "./bankBackend.h"

//using namespace std;
/*
std::map<unsigned long, float>* cache; // KIV : Eviction Policy/LRU ? Can use deque to track LRU account / LRU transfer ptr
std::unordered_set<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>*>* transactions;
*/
bankBackend::bankBackend() {
    cache = new std::map<unsigned long, float>(); // cache of account num to balance key-value pairs, for quick create, balance and transfer requests
    transactions = new std::unordered_set<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>*>();
}


std::map<unsigned long, float>::iterator bankBackend::findAccount(unsigned long account) {
    return cache->find(account);
}

// for balance queries
float bankBackend::getBalance(unsigned long account) {
    std::map<unsigned long, float>::iterator it = findAccount(account);//cache->find(account);
    if (it != cache->end()) {
        float foundBalance = it->second;
        printf("Current balance of account %lu : %f\n", account, foundBalance);
        return foundBalance;
    }
    printf("Account %lu not in cache\n", account);
    return -1;
}

// for creating/resetting accounts and transfers
bool bankBackend::setBalance(unsigned long account, float balance, bool reset) {
    std::map<unsigned long, float>::iterator it = findAccount(account);
    if (it == cache->end()) { // creating new account
        std::pair<std::map<unsigned long, float>::iterator, bool> inserted = cache->insert(std::pair<unsigned long, float>(account, balance));
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
void bankBackend::saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string>* tags) {
    std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>* transTup = new std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>(fromAccount, toAccount, amount, tags);
    transactions->insert(transTup);
    return;
}

// for transfers
bool bankBackend::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string>* tags) {
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
    std::map<unsigned long, float>::iterator fromIt = findAccount(from);
    std::map<unsigned long, float>::iterator toIt = findAccount(to);
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

// Since this is per-request state, it does not need persistence, hence object returned instead of allocated pointer, to facilitate exception safety
std::vector<std::tuple<bool, std::string>> bankBackend::createAccounts(std::vector<std::tuple<unsigned long, float, bool, std::string>> createReqs) {
    std::vector<std::tuple<bool, std::string>> resultVec;
    for (std::vector<std::tuple<unsigned long, float, bool, std::string>>::iterator it = createReqs.begin(); it < createReqs.end(); it ++) {
        std::tuple<unsigned long, float, bool, std::string> reqTuple = *it;
        bool createResult = setBalance(std::get<0>(reqTuple), std::get<1>(reqTuple), std::get<2>(reqTuple));
        resultVec.push_back(std::tuple<bool, std::string>(createResult, std::get<3>(reqTuple)));
    }
    return resultVec;
}

// Same as above
std::vector<std::tuple<float, std::string>> bankBackend::getBalances(std::vector<std::tuple<unsigned long, std::string>>  balanceReqs) {
    std::vector<std::tuple<float, std::string>> resultVec;
    for (std::vector<std::tuple<unsigned long, std::string>>::iterator it = balanceReqs.begin(); it < balanceReqs.end(); it ++) {
        std::tuple<unsigned long, std::string> reqTuple = *it;
        float balanceResult = getBalance(std::get<0>(reqTuple));
        resultVec.push_back(std::tuple<float, std::string>(balanceResult, std::get<1>(reqTuple)));
    }
    return resultVec;
}

/*
// for queries
unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> processORQuery(unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> querySet, vector<>) {

    return NULL;
}


unordered_set<tuple<unsigned long, unsigned long, float, vector<string>*>*> processQueryAmountConds() {

    return NULL;
}
*/


/*
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


    delete cache;
    return 0;
}
*/

          