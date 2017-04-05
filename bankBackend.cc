#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <queue>
#include "./bankBackend.h"
//#include "./bankDBHandler.h"

//using namespace std;
/*
std::map<unsigned long, float>* cache; // KIV : Eviction Policy/LRU ? Can use deque to track LRU account / LRU transfer ptr
std::unordered_set<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>*>* transactions;
*/
bankBackend::bankBackend() {
    cache = new std::map<unsigned long, float>(); // cache of account num to balance key-value pairs, for quick create, balance and transfer requests
    //transactions = new std::unordered_set<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>*>();
    dbHandler = new bankDBHandler();
}


std::map<unsigned long, float>::iterator bankBackend::findAccount(unsigned long account) {
    return cache->find(account);
}

// for balance queries
float bankBackend::getBalance(unsigned long account) {
    // if in cache, return cached value
    float foundBalance;
    std::map<unsigned long, float>::iterator it = findAccount(account);
    if (it != cache->end()) { // in cache
        foundBalance = it->second;
        printf("Current balance of account %lu : %f\n", account, foundBalance);
        return foundBalance;
    }
    // not in cache, check DB
    // TO-DO : will eventually use DB threadpool
    printf("Account %lu not in cache\n", account);
    pqxx::result getBalanceResult = dbHandler->getBalance(account);
    if (getBalanceResult.size() == 0 || getBalanceResult[0]["balance"].is_null()) { // not in DB either, must be non-existent account
        return -1;
    }
    if (!getBalanceResult[0]["balance"].to(foundBalance)) return - 1;
    return foundBalance;
}

// for creating/resetting accounts and transfers
bool bankBackend::setBalance(unsigned long account, float balance, bool reset) {
    std::map<unsigned long, float>::iterator it = findAccount(account);
    if (it != cache->end()) { // account exists
        if (!reset) { // account already exists and not a reset, do not update
            printf("Account %lu already exists, ignoring\n", account);
            return false;
        }
        // updating existing account's balance; update DB and write to cache to speed up future reads ('balance' requests)
        it->second = balance;
        printf("Overwrote account %lu's balance to %f\n", it->first, it->second);
        // Write-through to database after this
    }
    
    else { // new account; update DB and write to cache to speed up future reads ('balance' requests)     
        std::pair<std::map<unsigned long, float>::iterator, bool> inserted = cache->insert(std::pair<unsigned long, float>(account, balance));
        printf("Created new account %lu with balance %f\n", inserted.first->first, inserted.first->second);
        // Write-through to database after this
    }
    int commitSuccess = 0;
    pqxx::result setBalanceResult = dbHandler->saveAccount(account, balance, reset, &commitSuccess);
    if (!commitSuccess) { // writing to DB failed
        // TO-DO : Retry?
        return false; // unable to commit to DB, return error to client
    }
    return true; // successfully committed to DB
}




// for transfers
//void bankBackend::saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string>* tags) {
bool bankBackend::saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags) {
    //std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>* transTup = new std::tuple<unsigned long, unsigned long, float, std::vector<std::string>*>(fromAccount, toAccount, amount, tags);
    //std::tuple<unsigned long, unsigned long, float, std::vector<std::string>> transTup(fromAccount, toAccount, amount, tags); 
    int transferSuccess = 0;
    std::tuple<unsigned long, float, unsigned long, float> newBalances = dbHandler->transfer(fromAccount, toAccount, amount, tags, &transferSuccess);
    if (transferSuccess) { // update cache
        cache->insert(std::pair<unsigned long, float>(std::get<0>(newBalances), std::get<1>(newBalances)));
        cache->insert(std::pair<unsigned long, float>(std::get<2>(newBalances), std::get<3>(newBalances)));
    }
    // transfer failed, so cache is still valid
    return transferSuccess == 1;
}

// for transfers
//bool bankBackend::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string>* tags) {
bool bankBackend::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags) {
    /* NO POINT CHECKING IN CACHE FOR THIS
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
    */
    return saveTransfer(fromAccount, toAccount, amount, tags); // write to DB
    //return true;
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

// Same as above, for all balances in a request
std::vector<std::tuple<float, std::string>> bankBackend::getBalances(std::vector<std::tuple<unsigned long, std::string>>  balanceReqs) {
    std::vector<std::tuple<float, std::string>> resultVec;
    for (std::vector<std::tuple<unsigned long, std::string>>::iterator it = balanceReqs.begin(); it < balanceReqs.end(); it ++) {
        std::tuple<unsigned long, std::string> reqTuple = *it;
        float balanceResult = getBalance(std::get<0>(reqTuple));
        resultVec.push_back(std::tuple<float, std::string>(balanceResult, std::get<1>(reqTuple)));
    }
    return resultVec;
}

// Same as above, for all transfers in a request
std::vector<std::tuple<bool, std::string>> bankBackend::doTransfers(std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs) {
    std::vector<std::tuple<bool, std::string>> resultVec;
    for (std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>>::iterator it = transferReqs.begin(); it < transferReqs.end(); it ++ ) {
        std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>> reqTuple = *it;
        bool transferResult = transfer(std::get<0>(reqTuple), std::get<1>(reqTuple), std::get<2>(reqTuple), std::get<4>(reqTuple));
        resultVec.push_back(std::tuple<bool, std::string>(transferResult, std::get<3>(reqTuple)));
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
