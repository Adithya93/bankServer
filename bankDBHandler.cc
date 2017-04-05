#include <iostream>
#include <pqxx/pqxx>
#include "./bankDBHandler.h"
/* ADAPTED FROM/INSPIRED BY EXAMPLES AT PQXX.ORG*/

bankDBHandler::bankDBHandler() {
  // TO-DO : initialize threadpool
}

// Private helper method called by saveAccount and transfer, assumes account already exists, does not check if reset or account exists (since caller does that)
pqxx::result bankDBHandler::updateBalance(unsigned long account, float balance, int * result) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  std::string queryString("UPDATE accounts SET balance = " + std::to_string(balance) + " WHERE id = " + std::to_string(account));
  std::cout << "Updating balance of account no. " << account;
  std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  std::cout << "About to commit txn!\n";
  txn.commit();
  *result = 1; // Set success flag for caller
  std::cout << "Txn committed!\n";
  return r;
}

pqxx::result bankDBHandler::getBalance(unsigned long account) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  std::string queryString("SELECT * FROM accounts WHERE id = " + std::to_string(account));
  std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  
  std::cout << "About to present rows of table\n";
  if (r.size() == 0) {
    std::cout << "Null result\n";
    return r;
  }
  /*
  for (auto row: r)
    std::cout
      // Address column by name.  Use c_str() to get C-style string.
      << "Account no. "
      << row["id"].c_str()
      << " has "
      << row["balance"].as<int>()
      << "."
      << std::endl;
  */
  std::cout << "About to commit txn!\n";
  // Not needed for SELECT queries, but just a practice
  txn.commit();
  std::cout << "Txn committed!\n";

  // Connection object goes out of scope here.  It closes automatically.
  return r;
}

pqxx::result bankDBHandler::saveAccount(unsigned long account, float balance, bool reset, int* result) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  bool update = false;
  std::string testQueryString("SELECT * FROM accounts WHERE id = " + std::to_string(account));
  std::cout << "Test Query String: " << testQueryString << "\n";
  pqxx::result testResult = txn.exec(testQueryString);    
  if (testResult.size() > 0) { // already exists
    if (!reset) {
      std::cout << "Account already exists, aborting create request\n";
      return testResult;
    }
    else {
      update = true;
    }
  }
  std::string queryString;
  if (!update) {
    queryString = "INSERT INTO accounts (id, balance) VALUES (" + std::to_string(account) + ", " + std::to_string(balance) + ")";//\n WHERE NOT EXISTS (SELECT * FROM accounts WHERE id = " + std::to_string(account) + ")");
    std::cout << "Query String: " << queryString << "\n";
    pqxx::result r = txn.exec(queryString);
    std::cout << "About to commit txn!\n";
    txn.commit();
    *result = 1; // Set success flag for caller
    std::cout << "Txn committed!\n";
    return r;
  }
  else {
    /*
    queryString = "UPDATE accounts SET balance = " + std::to_string(balance) + " WHERE id = " + std::to_string(account);
    std::cout << "Updating balance of account no. " << account;
    */
    int updateSuccess = 0;
    pqxx::result updateResult = updateBalance(account, balance, &updateSuccess);
    *result = updateSuccess; // if updateBalance succeeded, so did this method, and vice-versa
    return updateResult;
  }

}

// Validation done beforehand
// TO-DO : MUST RETURN LATEST BALANCES OF BOTH ACCOUNTS (whether success or failure) SO THAT BANK BACKEND CAN UPDATE ITS CACHE!!! ELSE CACHE INVALIDATED!
pqxx::result bankDBHandler::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int* success) {
  // getBalance of fromAccount --> fromBalance; if not exists, *result = 0; return failure
  pqxx::result fromAccountResult = getBalance(fromAccount);
  if (fromAccountResult.size() == 0) { // fromAccount does not exist
    *success = 0; // fail
    std::cout << "FROM account of transfer does not exist, aborting\n";
    return fromAccountResult;
  }
  float fromBalance;
  if (fromAccountResult[0]["balance"].is_null() || !fromAccountResult[0]["balance"].to(fromBalance)) { // unable to extract float balance
    *success = 0; // fail
    // TO-DO : Must log this as error as it represents server logic failure
    return fromAccountResult;
  }

  // getBalance of toAccount --> toBalance; if not exists, *result = 0; return failure
  pqxx::result toAccountResult = getBalance(toAccount);
  if (toAccountResult.size() == 0) { // toAccount does not exist
    *success = 0; // fail
    std::cout << "TO account of transfer does not exist, aborting\n";
    return toAccountResult;
  }
  float toBalance;
  if (toAccountResult[0]["balance"].is_null() || !toAccountResult[0]["balance"].to(toBalance)) { // unable to extract float balance
    *success = 0; // fail
    // TO-DO : Must log this as error as it represents server logic failure
    return toAccountResult;
  }

  // verify (fromBalance - amount) > 0 && (toBalance + amount) > 0 ; else return failure
  if ((fromBalance - amount < 0) || (toBalance + amount < 0)) { // insufficient funds
    std::cout << "Insufficient funds for transfer, aborting\n";
    *success = 0; // fail
    return toAccountResult;
  }

  // updateAccount of fromAccount with fromBalance - amount ; if failure return failure
  int updateSuccess = 0;
  pqxx::result updateResult = updateBalance(fromAccount, fromBalance - amount, &updateSuccess);
  if (!updateSuccess) {
    std::cout << "Unable to deduct amount from sender of transfer, aborting\n";
    *success = 0; // fail
    return updateResult;
  }

  // updateAccount of toAccount with toBalance + amount ; if failure return failure
  updateSuccess = 0;
  updateResult = updateBalance(toAccount, toBalance + amount, &updateSuccess);
  if (!updateSuccess) {
    std::cout << "Unable to add amount to recipient of transfer, aborting\n";
    *success = 0; // fail
    return updateResult;
  }
  // return success
  *success = 1;
  return updateResult;
}
