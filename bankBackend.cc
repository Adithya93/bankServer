#include "./bankBackend.h"

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

void bankBackend::cleanUp() {
    delete cache;
    delete dbHandler;
}

std::tuple<std::map<unsigned long, float>::iterator, bool> bankBackend::findAccount(unsigned long account) { // return tuple so that caller doesn't have to reacquire lock
    std::lock_guard<std::mutex> guard(cacheMutex);
    std::map<unsigned long, float>::iterator it = cache->find(account);
    return std::tuple<std::map<unsigned long, float>::iterator, bool>(it, it == cache->end()); // should automatically unlock as the lock guard goes out of scope
}

// for balance queries
float bankBackend::getBalance(unsigned long account) {
    // if in cache, return cached value
    float foundBalance;
    std::tuple<std::map<unsigned long, float>::iterator, bool> cacheTup = findAccount(account);
    std::map<unsigned long, float>::iterator it = std::get<0>(cacheTup);
    if (!std::get<1>(cacheTup)) { // in cache
        foundBalance = it->second;
        //printf("Current balance of account %lu : %f\n", account, foundBalance);
        //std::cout << "Returning cached value of " << foundBalance << " for account " << account << "\n";
        return foundBalance;
    }
    // not in cache, check DB
    // TO-DO : will eventually use DB threadpool
    //printf("Account %lu not in cache\n", account);
    pqxx::result getBalanceResult = dbHandler->getBalance(account);
    if (getBalanceResult.size() == 0 || getBalanceResult[0]["balance"].is_null()) { // not in DB either, must be non-existent account
        return -1;
    }
    if (!getBalanceResult[0]["balance"].to(foundBalance)) return - 1;
    return foundBalance;
}

// for creating/resetting accounts and transfers
bool bankBackend::setBalance(unsigned long account, float balance, bool reset, bool writtenToDB) {
    std::tuple<std::map<unsigned long, float>::iterator, bool> cacheTup = findAccount(account);
    std::map<unsigned long, float>::iterator it = std::get<0>(cacheTup);
    if (!std::get<1>(cacheTup)) { // account in cache
        if (!reset) { // account already exists and not a reset, do not update
            //printf("Account %lu already exists, ignoring\n", account);
            return false;
        }
        // updating existing account's balance; update DB and write to cache to speed up future reads ('balance' requests)
        std::lock_guard<std::mutex> guard(cacheMutex);
        it->second = balance;
        //printf("Overwrote account %lu's balance to %f\n", it->first, it->second);
        // Write-through to database after this
    }
    
    else { // new account; update DB and write to cache to speed up future reads ('balance' requests)     
        std::lock_guard<std::mutex> guard(cacheMutex);
        std::pair<std::map<unsigned long, float>::iterator, bool> inserted = cache->insert(std::pair<unsigned long, float>(account, balance));
        //printf("Created new account %lu with balance %f\n", inserted.first->first, inserted.first->second);
        // Write-through to database after this
    }
    if (writtenToDB) {
        //std::cout << "Already written to DB, returning success\n";
        return true;
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
int bankBackend::saveTransfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags) {
    int transferSuccess = 0;
    std::tuple<unsigned long, float, unsigned long, float> newBalances = dbHandler->transfer(fromAccount, toAccount, amount, tags, &transferSuccess);
    if (transferSuccess == 1) { // update cache
        //std::lock_guard<std::mutex> guard(cacheMutex);
        //std::cout << "Updating cache for account " << std::get<0>(newBalances) << " to have value of " << std::get<1>(newBalances) << "\n";
        //std::cout << "Updating cache for account " << std::get<2>(newBalances) << " to have value of " << std::get<3>(newBalances) << "\n";
        //cache->insert(std::pair<unsigned long, float>(std::get<0>(newBalances), std::get<1>(newBalances)));
        setBalance(std::get<0>(newBalances), std::get<1>(newBalances), true, true);
        setBalance(std::get<2>(newBalances), std::get<3>(newBalances), true, true);

        //cache->insert(std::pair<unsigned long, float>(std::get<2>(newBalances), std::get<3>(newBalances)));
    }
    // transfer failed, so cache is still valid
    return transferSuccess;
}

// for transfers
int bankBackend::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags) {
    /* NO POINT CHECKING IN CACHE FOR THIS
    */
    return saveTransfer(fromAccount, toAccount, amount, tags); // write to DB
}

std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> bankBackend::query(std::string queryString) {
    return dbHandler->query(queryString);
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
std::vector<std::tuple<int, std::string>> bankBackend::doTransfers(std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>> transferReqs) {
    //std::vector<std::tuple<bool, std::string>> resultVec;
    std::vector<std::tuple<int, std::string>> resultVec;
    for (std::vector<std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>>>::iterator it = transferReqs.begin(); it < transferReqs.end(); it ++ ) {
        std::tuple<unsigned long, unsigned long, float, std::string, std::vector<std::string>> reqTuple = *it;
        //bool transferResult = transfer(std::get<0>(reqTuple), std::get<1>(reqTuple), std::get<2>(reqTuple), std::get<4>(reqTuple));
        int transferResult = transfer(std::get<0>(reqTuple), std::get<1>(reqTuple), std::get<2>(reqTuple), std::get<4>(reqTuple));
        //resultVec.push_back(std::tuple<bool, std::string>(transferResult, std::get<3>(reqTuple)));
        resultVec.push_back(std::tuple<int, std::string>(transferResult, std::get<3>(reqTuple)));
    }
    return resultVec;
}

std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> bankBackend::doQueries(std::vector<std::tuple<std::string, std::string>> queryReqs) {
    std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>> resultVec;
    for (std::vector<std::tuple<std::string, std::string>>::iterator it = queryReqs.begin(); it < queryReqs.end(); it ++) {
        std::tuple<std::string, std::string> reqTuple = *it;
        if (std::get<0>(reqTuple).size() == 0) { // invalid query
            std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> emptyResult;
            resultVec.push_back(std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>(false, std::get<1>(reqTuple), emptyResult));
        }
        else {
            std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> queryResult = query(std::get<0>(reqTuple));
            resultVec.push_back(std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>(true, std::get<1>(reqTuple), queryResult));
        }
    }
    return resultVec;
}

