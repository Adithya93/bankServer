#include <iostream>
#include <pqxx/pqxx>
/* ADAPTED FROM EXAMPLES AT PQXX.ORG*/

/// Insert accounts into database.
pqxx::result saveAccount(unsigned long account, float balance, bool reset, int* result) {
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
  }
  else {
    queryString = "UPDATE accounts SET balance = " + std::to_string(balance) + " WHERE id = " + std::to_string(account);
    std::cout << "Updating balance of account no. " << account;
  }

  std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  std::cout << "About to commit txn!\n";
  txn.commit();
  *result = 1; // Set success flag for caller
  std::cout << "Txn committed!\n";
  return r;
}


/// Query employees from database, print results.
int main(int argc, char *argv[])
{
  try
  {
    if (argc < 4) {
      std::cout << "Usage: ./testDB <account_num> <balance> <reset>\n";
      exit(1);
    }
    unsigned long account = strtoul(argv[1], NULL, 10);
    float balance = std::stof(argv[2]);
    int reset = atoi(argv[3]);

    int success = 0;
    pqxx::result r = saveAccount(account, balance, reset, &success);
    if (!success) {
      std::cout << "Create request failed!\n";
      return 0;
    }
    // Results can be accessed and iterated again.  Even after the connection
    // has been closed.
    for (auto row: r)
    {
      std::cout << "Row: ";
      // Iterate over fields in a row.
      for (auto field: row) std::cout << field.c_str() << " ";
      std::cout << std::endl;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  catch (const pqxx::sql_error &e)
  {
    std::cerr << "SQL error: " << e.what() << std::endl;
    std::cerr << "Query was: " << e.query() << std::endl;
    return 2;
  }
  return 0;
}