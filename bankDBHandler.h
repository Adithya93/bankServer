#include <iostream>
#include <pqxx/pqxx>
#include <vector>
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
    std::tuple<unsigned long, float, unsigned long, float> transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int * result);

    std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>> query(std::string queryString); 

  private:
    pqxx::result updateBalance(unsigned long account, float balance, int * result);

    pqxx::result updateBalances(unsigned long fromAccount, float fromBalanceNew, unsigned long toAccount, float toBalanceNew, int* success); 

    pqxx::result saveTxn(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int* success);

    pqxx::result makeQuery(std::string queryString);

    std::vector<std::string> getTags(unsigned long txnID);

    float MIN_BALANCE = 0;
    int INSUFFICIENT_FUNDS = -1;
    int INVALID_ACCOUNT = -2;

  
};
