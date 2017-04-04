#include <iostream>
#include <pqxx/pqxx>
/* ADAPTED FROM EXAMPLES AT PQXX.ORG*/

/// Query accounts from database.  Return result.
pqxx::result getBalance(unsigned long account) {
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

  for (auto row: r)
    std::cout
      // Address column by name.  Use c_str() to get C-style string.
      << "Account no. "
      << row["id"].c_str()
      << " has "
      << row["balance"].as<int>()
      << "."
      << std::endl;

  std::cout << "About to commit txn!\n";
  // Not needed for SELECT queries, but just a practice
  txn.commit();
  std::cout << "Txn committed!\n";

  // Connection object goes out of scope here.  It closes automatically.
  return r;
}


/// Query employees from database, print results.
int main(int argc, char *argv[])
{
  try
  {
      if (argc < 2) {
        std::cout << "Usage: ./testDBSelect <account_num>\n";
        exit(1);
      }
    unsigned long account = strtoul(argv[1], NULL, 10);

    pqxx::result r = getBalance(account);

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
}