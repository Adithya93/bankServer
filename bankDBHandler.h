#include <iostream>
#include <pqxx/pqxx>
/* ADAPTED FROM EXAMPLES AT PQXX.ORG*/

class bankDBHandler {

  public:

    // constructor
    bankDBHandler();

    /// Insert accounts into database
    pqxx::result saveAccount(unsigned long account, float balance, bool reset, int* result);

    // Get balance from database
    pqxx::result getBalance(unsigned long account);

    // Update accounts in database
    pqxx::result transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int * result);

  private:
    pqxx::result updateBalance(unsigned long account, float balance, int * result);
  
};
