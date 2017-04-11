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
  //std::cout << "Updating balance of account no. " << account;
  //std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  //std::cout << "About to commit txn!\n";
  txn.commit();
  *result = 1; // Set success flag for caller
  //std::cout << "Txn committed!\n";
  return r;
}

pqxx::result bankDBHandler::getBalance(unsigned long account) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  std::string queryString("SELECT * FROM accounts WHERE id = " + std::to_string(account));
  //std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  
  //std::cout << "About to present rows of table\n";
  if (r.size() == 0) {
    std::cout << "Null result for lookup of account for balance\n";
    return r;
  }
  //std::cout << "About to commit txn!\n";
  // Not needed for SELECT queries, but just a practice
  txn.commit();
  //std::cout << "Txn committed!\n";

  return r;
}

pqxx::result bankDBHandler::saveAccount(unsigned long account, float balance, bool reset, int* result) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  bool update = false;
  std::string testQueryString("SELECT * FROM accounts WHERE id = " + std::to_string(account));
  //std::cout << "Test Query String: " << testQueryString << "\n";
  pqxx::result testResult = txn.exec(testQueryString);    
  if (testResult.size() > 0) { // already exists
    if (!reset) {
      //std::cout << "Account already exists, aborting create request\n";
      return testResult;
    }
    else {
      update = true;
    }
  }
  std::string queryString;
  if (!update) {
    queryString = "INSERT INTO accounts (id, balance) VALUES (" + std::to_string(account) + ", " + std::to_string(balance) + ")";//\n WHERE NOT EXISTS (SELECT * FROM accounts WHERE id = " + std::to_string(account) + ")");
    //std::cout << "Query String: " << queryString << "\n";
    pqxx::result r = txn.exec(queryString);
    //std::cout << "About to commit txn!\n";
    txn.commit();
    *result = 1; // Set success flag for caller
    //std::cout << "Txn committed!\n";
    return r;
  }
  else {
    /*
    queryString = "UPDATE accounts SET balance = " + std::to_string(balance) + " WHERE id = " + std::to_string(account);
    std::cout << "Updating balance of account no. " << account;
    */
    //int updateSuccess = 0;
    //pqxx::result updateResult = updateBalance(account, balance, &updateSuccess);
    pqxx::result updateResult = updateBalance(account, balance, result);
    //*result = updateSuccess; // if updateBalance succeeded, so did this method, and vice-versa
    return updateResult;
  }

}


pqxx::result bankDBHandler::updateBalances(unsigned long fromAccount, float fromBalanceNew, unsigned long toAccount, float toBalanceNew, int* success) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  // From
  std::string queryStringFrom("UPDATE accounts SET balance = " + std::to_string(fromBalanceNew) + " WHERE id = " + std::to_string(fromAccount));
  //std::cout << "Updating balance of account no. " << fromAccount;
  //std::cout << "Query String: " << queryStringFrom << "\n";
  pqxx::result rFrom = txn.exec(queryStringFrom);
  // To
  std::string queryStringTo("UPDATE accounts SET balance = " + std::to_string(toBalanceNew) + " WHERE id = " + std::to_string(toAccount));
  //std::cout << "Updating balance of account no. " << toAccount;
  //std::cout << "Query String: " << queryStringTo << "\n";
  pqxx::result rTo = txn.exec(queryStringTo);
  //std::cout << "About to commit txn!\n";
  txn.commit();
  *success = 1; // Set success flag for caller
  //std::cout << "Txn committed!\n";
  return rTo;
}


/* NEED TO MAKE SAVING OX TXN AND SAVING OF TAG ATOMIC!
// INSERTING MULTIPLE VALUES IN 1 INSERT QUERY IS FASTER THAN RUNNING MULTIPLE SINGLE-VALUE INSERT QUERIES
// NOTE : LIMIT OF 1000 VALUES IN A SINGLE INSERT
*/

/*
pqxx::result bankDBHandler::saveTags(unsigned long transferID, std::vector<std::string> tags, pqxx::work * txnRef, int* success) {
  
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  
  pqxx::work txn = *txnRef;
  std::string queryString("INSERT INTO tags (tag, txnID) VALUES\n");
  std::string transferIDStr = std::to_string(transferID);
  for (std::vector<std::string>::iterator it = tags.begin(); it < tags.end() - 1; it ++) {
    std::string tag = *it;
    queryString.append("(" + tag + ", " + transferIDStr + "), ");
  }
  queryString.append("(" + *(tags.end() - 1) + ", " + transferIDStr + ")");
  std::cout << "Query string:\n" << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  std::cout << "About to commit txn!\n";
  txn.commit();
  *success = 1; // Set success flag for caller
  std::cout << "Txn committed!\n";
  return r;
}
*/

pqxx::result bankDBHandler::saveTxn(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int* success) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};

  std::string queryString("INSERT INTO txns (f, t, a)\nVALUES\n(" + std::to_string(fromAccount) + ", " + std::to_string(toAccount) + ", " + std::to_string(amount) + ")\nRETURNING txns.id;");
  //std::cout << "Query string:\n" << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  //std::cout << "About to commit txn!\n";

  unsigned long txnID;
  if (r.size() == 0 || r[0]["ID"].is_null() || !r[0]["ID"].to(txnID)) {
    // Can't save tags, don't commit
    *success = 0; // fail
    return r;
  }
  // Save Tags, if any
  if (tags.size() > 0) {

    std::string queryString("INSERT INTO tags (tag, txnid) VALUES\n");
    std::string transferIDStr = std::to_string(txnID);
    for (std::vector<std::string>::iterator it = tags.begin(); it < tags.end() - 1; it ++) {
      std::string tag = *it;
      queryString.append("('" + tag + "', " + transferIDStr + "), ");
    }
    queryString.append("('" + *(tags.end() - 1) + "', " + transferIDStr + ")");
    //std::cout << "Query string:\n" << queryString << "\n";
    pqxx::result r = txn.exec(queryString);
    // TO-DO : Check result r of execution?
    //std::cout << "About to commit txn!\n";
  }
  //else {
    //std::cout << "No tags to save for this transfer\n";
  //}
  txn.commit();
  *success = 1; // Set success flag for caller
  //std::cout << "Txn committed!\n";
  return r;
}

pqxx::result bankDBHandler::makeQuery(std::string queryString) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  pqxx::result r = txn.exec(queryString);

  //std::cout << "No. of txns found : " << r.size() << "\n";
  /*
  for (int txnNo = 0; txnNo < r.size(); txnNo ++) {
    std::cout << "f: " << r[txnNo]["f"] << "; t: " << r[txnNo]["t"] << "; a" << r[txnNo]["a"] << "; id: " << r[txnNo]["id"] << "\n";
  }
  */
  txn.commit();
  return r;
}
/*
*/
std::vector<std::string> bankDBHandler::getTags(unsigned long txnID) {
  pqxx::connection c{"dbname=bankServer user=radithya"};
  pqxx::work txn{c};
  std::string queryString("SELECT * FROM tags WHERE txnid=" + std::to_string(txnID));
  //std::cout << "Query String is " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  std::vector<std::string> tags;

  if (r.size() == 0) {
    //std::cout << "No tags found for txn " << txnID << "\n";
  }
  else {
    //std::cout << "About to present rows of table\n";
    for (int row = 0; row < r.size(); row ++) {
      //std::cout << "Tag: " << r[row]["tag"] << "\n";
      std::string tagStr;
      if (!r[row]["tag"].to(tagStr)) {
        std::cout << "Unable to convert tag field to string; skipping\n";
        continue;
      }
      tags.push_back(tagStr);
    }
  }
  txn.commit();
  return tags;
}



// Validation done beforehand
// TO-DO : MUST RETURN LATEST BALANCES OF BOTH ACCOUNTS (whether success or failure) SO THAT BANK BACKEND CAN UPDATE ITS CACHE!!! ELSE CACHE INVALIDATED!
std::tuple<unsigned long, float, unsigned long, float> bankDBHandler::transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int* success) {
  // getBalance of fromAccount --> fromBalance; if not exists, *result = 0; return failure
  pqxx::result fromAccountResult = getBalance(fromAccount);
  if (fromAccountResult.size() == 0) { // fromAccount does not exist
    //*success = 0; // fail
    *success = INVALID_ACCOUNT; // fail
    std::cout << "FROM account of transfer does not exist, aborting\n";
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
    //return fromAccountResult;
  }
  float fromBalance;
  if (fromAccountResult[0]["balance"].is_null() || !fromAccountResult[0]["balance"].to(fromBalance)) { // unable to extract float balance
//  *success =0; //fail  
    *success = INVALID_ACCOUNT; // fail
    // TO-DO : Must log this as error as it represents server logic failure
    //return fromAccountResult;
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
  }

  // getBalance of toAccount --> toBalance; if not exists, *result = 0; return failure
  pqxx::result toAccountResult = getBalance(toAccount);
  if (toAccountResult.size() == 0) { // toAccount does not exist
//    *success = 0; // fail
    *success = INVALID_ACCOUNT; // fail
    std::cout << "TO account of transfer does not exist, aborting\n";
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
    //return toAccountResult;
  }
  float toBalance;
  if (toAccountResult[0]["balance"].is_null() || !toAccountResult[0]["balance"].to(toBalance)) { // unable to extract float balance
    //*success = 0; // fail
    *success = INVALID_ACCOUNT; // fail
    // TO-DO : Must log this as error as it represents server logic failure
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
    //return toAccountResult;
  }

  // verify (fromBalance - amount) > 0 && (toBalance + amount) > 0 ; else return failure
  if ((fromBalance - amount < 0) || (toBalance + amount < 0)) { // insufficient funds
    std::cout << "Insufficient funds for transfer, aborting\n";
//    *success = 0; // fail
    *success = INSUFFICIENT_FUNDS; // fail
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
    //return toAccountResult;
  }

  // atomically update both from and to accounts
  int updateSuccess = 0;
  pqxx::result updateResult = updateBalances(fromAccount, fromBalance - amount, toAccount, toBalance + amount, &updateSuccess);
  if (!updateSuccess) {
    std::cout << "Unable to transfer, aborting\n";
//    *success = 0; // fail
    *success = INSUFFICIENT_FUNDS; // fail
    return std::tuple<unsigned long, float, unsigned long, float>(0, 0, 0, 0); // value will be ignored
    //return updateResult;
  }

  // NOTE : THIS TAKES THE RISK OF ACCOUNTS BEING SAVED WITHOUT TAGS.... IS THIS ALRIGHT? MAYBE CHANGE TO INCLUDE THIS IN ABOVE TXN TOO?
  // Save to txn table  
  // UNCOMMENT WHEN ATOMICITY RESTORED
  pqxx::result txnSaveResult = saveTxn(fromAccount, toAccount, amount, tags, success);
  // return success
  //*success = 1;
  return std::tuple<unsigned long, float, unsigned long, float>(fromAccount, fromBalance - amount, toAccount, toBalance + amount); // value will be written into cache
  //return updateResult;
}


std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> bankDBHandler::query(std::string queryString) {
  std::string txnSelectPrefix("SELECT * FROM txns WHERE ");
  std::string txnEnd(";");
  txnSelectPrefix += queryString + txnEnd;
  pqxx::result queryResult = makeQuery(txnSelectPrefix);
  std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> results;
  unsigned long from, to, id;
  float amount;

  for (int resultNum = 0; resultNum < queryResult.size(); resultNum ++) {
    if (!queryResult[resultNum]["f"].to(from)) {
      std::cout << "Unable to retrieve 'from' field for txn, skipping\n";
      continue;
    }
    if (!queryResult[resultNum]["t"].to(to)) {
      std::cout << "Unable to retrieve 'to' field for txn, skipping\n";
      continue;
    }
    if (!queryResult[resultNum]["a"].to(amount)) {
      std::cout << "Unable to retrieve 'amount' field for txn, skipping\n";
      continue;
    }
    if (!queryResult[resultNum]["id"].to(id)) {
      std::cout << "Unable to retrieve 'amount' field for txn, skipping\n";
      continue;
    }  
    std::vector<std::string> tagsFound = getTags(id);
    results.push_back(std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>(from, to, amount, tagsFound));
  }
  return results;
}
