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
    std::tuple<unsigned long, float, unsigned long, float> transfer(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int * result);


  private:
    pqxx::result updateBalance(unsigned long account, float balance, int * result);

    pqxx::result updateBalances(unsigned long fromAccount, float fromBalanceNew, unsigned long toAccount, float toBalanceNew, int* success); 

    //pqxx::result saveTags(unsigned long transferID, std::vector<std::string> tags, pqxx::work* txnRef, int* success); 

    pqxx::result saveTxn(unsigned long fromAccount, unsigned long toAccount, float amount, std::vector<std::string> tags, int* success);

    float MIN_BALANCE = 0;
    int INSUFFICIENT_FUNDS = -1;
    int INVALID_ACCOUNT = -2;

  
};
