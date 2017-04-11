#include <time.h>
#include <cstdint>
#include <random>
#include <vector>
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <future>

#define MAX_UINT 18446744073709551615
#define PORT 

// generates a random integer between 0 and n inclusive
int RNG(int n){
  std::random_device r;
  std::default_random_engine generator(r());
  std::uniform_int_distribution<int> dist(0, n);
  int temp = dist(generator);
  return temp;
}

// generates a random number between 0 and MAX_UINT
uint64_t RNG64(){
  std::random_device r;
  std::default_random_engine generator(r());
  std::uniform_int_distribution<uint64_t> dist(0, MAX_UINT);
  uint64_t temp = dist(generator);
  return temp;
}

// generates a random number from a normal distribution with mean "mean" and standard deviation "stdev"
double RNGnormal(double mean, double stdev){
  std::random_device r;
  std::default_random_engine generator(r());
  std::normal_distribution<double> dist(mean, stdev);
  double temp = dist(generator);
  return temp;
}

//use random number generator to determine whether "reset == true" or "reset == false" or nothing at all
void reset(std::ofstream &toFile){
  int resetTrue = RNG(1);
  //"reset == true" or "reset == false". 1 means reset == true and 0 means reset == false
  if(resetTrue == 1){
    int trueOrFalse = RNG(1);
    //reset == false
    if(trueOrFalse == 0){
      //std::cout << "reset = False" << std::endl;
      toFile << "<transactions reset=\"false\">" << std::endl;
    }
    //reset == true
    else{
      //std::cout << "reset = True" << std::endl;
      toFile << "<transactions reset=\"true\">" << std::endl;
    }
  }
  else{
    //std::cout << "nothing for reset" << std::endl;
    toFile << "<transactions>" << std::endl;
  }
}

//create a whole bunch of accounts first
void createRequest(std::vector<uint64_t> &accounts, std::ofstream &toFile,int i){
  // create account number first
  uint64_t accountNumber = RNG64();
  accounts.push_back(accountNumber);
  
  //std::cout << "account number: " << accountNumber << std::endl;
  
  // should we include balance tag present?
  int balance = RNG(1);
  // yes
  toFile << "\t" << "<create ref=\"c" << i << "\">" << std::endl;
  toFile << "\t\t" << "<account>" << accountNumber << "</account>" << std::endl; 
  if(balance == 1){
    // now create a random valid floating point number
    double money = RNGnormal(50000, 20000);
    while(money < 0){
      money = RNGnormal(50000, 5000);
    }
    //std::cout << "balance: $" << money << std::endl;
    toFile << "\t\t" << "<balance>" << money << "</balance>" << std::endl;
  }
  else{
    //std::cout << "0 balance" << std::endl;
    // no balance tag, which means bank account is 0
  }
  toFile << "\t" << "</create>" << std::endl;
}

void transferRequest(std::vector<uint64_t> &accounts, std::ofstream &toFile, int i){
  // first, test with accounts that we know are in the system using the "accounts" vector
  uint64_t accountNumber1 = accounts[RNG(accounts.size())];
  uint64_t accountNumber2 = accounts[RNG(accounts.size())];
  // next, generate a random amount
  double transferAmount = RNGnormal(3000.00,2000.00);
  while(transferAmount < 0){
    transferAmount = RNGnormal(3000.00, 2000.00);
  }

  //std::cout << "from " << accountNumber1 << " to " << accountNumber2 << " for the amount of " << transferAmount << std::endl;
  toFile << "\t" << "<transfer ref=\"" << i << "\">" << std::endl;
  toFile << "\t\t" << "<to>" << accountNumber1 << "</to>" << std::endl;
  toFile << "\t\t" << "<from>" << accountNumber2 << "</from>" << std::endl;
  toFile << "\t\t" << "<amount>" << transferAmount << "</amount>" << std::endl;
  toFile << "\t" << "</transfer>" << std::endl;
  
  // now let's generate random account numbers. These may or may not be accounts that we have already created
  uint64_t accountNumber3 = RNG64();
  uint64_t accountNumber4 = RNG64();
  double transferAmount2 = RNGnormal(5000.00, 400.00);
  while(transferAmount2 < 0){
    transferAmount2 = RNGnormal(3000.00, 500.00);
  }
  //std::cout << "from " << accountNumber3 << " to " << accountNumber4 << " for the amount of " << transferAmount2 << std::endl;
  toFile << "\t" << "<transfer ref=\"" << i+1 << "\">" << std::endl;
  toFile << "\t\t" << "<to>" << accountNumber1 << "</to>" << std::endl;
  toFile << "\t\t" << "<from>" << accountNumber2 << "</from>" << std::endl;
  toFile << "\t\t" << "<amount>" << transferAmount << "</amount>" << std::endl;
  toFile << "\t\t" << "<tag>" << "something" << "</tag>" << std::endl;
  toFile << "\t" << "</transfer>" << std::endl;
}

void balanceRequest(std::vector<uint64_t> &accounts, std::ofstream &toFile, int i){
  // first, test with an acount number that we know exists with the "accounts" vector
  uint64_t accountNumber1 = accounts[RNG(accounts.size())];
  //std::cout << "balance is: " << accountNumber1 << std::endl;
  toFile << "\t" << "<balance ref=\""<< i << "\">" << std::endl;
  toFile << "\t\t" << "<account>" << accountNumber1 << "</account>" << std::endl;
  toFile << "\t" << "</balance>" << std::endl;
  // next, test with an account number that may or not be in the system
  uint64_t accountNumber2 = RNG64();
  //std::cout << "balance is: " << accountNumber2 << std::endl;
  toFile << "\t" << "<balance ref=\""<< i+1 << "\">" << std::endl;
  toFile << "\t\t" << "<account>" << accountNumber2 << "</account>" << std::endl;
  toFile << "\t" << "</balance>" << std::endl;
}

void queryRequest(std::vector<uint64_t> &accounts, std::ofstream &toFile, int i){
  int amount = RNG(1000);
  // for "or" and "equals"
  uint64_t accountNumber1 = accounts[RNG(accounts.size())];
  uint64_t accountNumber2 = accounts[RNG(accounts.size())];
  // XML

  toFile << "\t" << "<query ref = \"" << i << "\">" << std::endl;
  toFile << "\t\t" << "<or>" << std::endl;
  toFile << "\t\t\t" << "<equals from=\"" << accountNumber1 << "\"/>" << std::endl;
  toFile << "\t\t\t" << "<equals to=\"" << accountNumber2 << "\"/>" << std::endl;
  toFile << "\t\t" << "</or>" << std::endl;
  toFile << "\t\t" << "<equal amount=\"" << amount << "\"/>" << std::endl;
  toFile << "\t" << "</query>" << std::endl;
  

  // for "and" and "less"
  uint64_t accountNumber3 = accounts[RNG(accounts.size())];
  uint64_t accountNumber4 = accounts[RNG(accounts.size())];
  // XML

  toFile << "\t" << "<query ref = \"" << i+1 << "\">" << std::endl;
  toFile << "\t\t" << "<and>" << std::endl;
  toFile << "\t\t\t" << "<less from=\"" << accountNumber3 << "\"/>" << std::endl;
  toFile << "\t\t\t" << "<less to=\"" << accountNumber4 << "\"/>" << std::endl;
  toFile << "\t\t" << "</and>" << std::endl;
  toFile << "\t\t" << "<less amount=\"" << amount << "\"/>" << std::endl;
  toFile << "\t" << "</query>" << std::endl;
  
  // for "not" and "greater"
  uint64_t accountNumber5 = accounts[RNG(accounts.size())];
  uint64_t accountNumber6 = accounts[RNG(accounts.size())];
  // XML

  toFile << "\t" << "<query ref = \"" << i+2 << "\">" << std::endl;
  toFile << "\t\t" << "<not>" << std::endl;
  toFile << "\t\t\t" << "<greater from=\"" << accountNumber5 << "\"/>" << std::endl;
  toFile << "\t\t\t" << "<greater to=\"" << accountNumber6 << "\"/>" << std::endl;
  toFile << "\t\t" << "</not>" << std::endl;
  toFile << "\t\t" << "<greater amount=\"" << amount << "\"/>" << std::endl;
  toFile << "\t" << "</query>" << std::endl;
  
  //generate some random test cases
  int chooseLogic = RNG(2);
  int chooseRelation = RNG(2);
  int amount2 = RNG(2000);
  //std::cout << "chooseLogic: " << chooseLogic << " chooseRelation: " << chooseRelation << std::endl;
  
  uint64_t accountNumber7 = accounts[RNG(accounts.size())];
  uint64_t accountNumber8 = accounts[RNG(accounts.size())];

  // XML for each of these sub-cases

  // "or"
  if(chooseLogic == 0){
    // "equal"
    if(chooseRelation == 0){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<or>" << std::endl;
      toFile << "\t\t\t" << "<equal from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<equal to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</or>" << std::endl;
      toFile << "\t\t" << "<equal amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "less"
    else if(chooseRelation == 1){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<or>" << std::endl;
      toFile << "\t\t\t" << "<less from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<less to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</or>" << std::endl;
      toFile << "\t\t" << "<less amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "greater"
    else{
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<or>" << std::endl;
      toFile << "\t\t\t" << "<greater from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<greater to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</or>" << std::endl;
      toFile << "\t\t" << "<greater amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
  }
  // "and"
  else if(chooseLogic == 1){
    // "equal"
    if(chooseRelation == 0){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<and>" << std::endl;
      toFile << "\t\t\t" << "<equal from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<equal to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</and>" << std::endl;
      toFile << "\t\t" << "<equal amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "less"
    else if(chooseRelation == 1){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<and>" << std::endl;
      toFile << "\t\t\t" << "<less from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<less to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</and>" << std::endl;
      toFile << "\t\t" << "<less amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "greater"
    else{
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<and>" << std::endl;
      toFile << "\t\t\t" << "<greater from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<greater to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</and>" << std::endl;
      toFile << "\t\t" << "<greater amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
  }
  // "not"
  else{
    // "equal"
    if(chooseRelation == 0){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<not>" << std::endl;
      toFile << "\t\t\t" << "<equal from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<equal to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</not>" << std::endl;
      toFile << "\t\t" << "<equal amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "less"
    else if(chooseRelation == 1){
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<not>" << std::endl;
      toFile << "\t\t\t" << "<less from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<less to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</not>" << std::endl;
      toFile << "\t\t" << "<less amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
    // "greater"
    else{
      toFile << "\t" << "<query ref = \"" << i+3 << "\">" << std::endl;
      toFile << "\t\t" << "<not>" << std::endl;
      toFile << "\t\t\t" << "<greater from=\"" << accountNumber7 << "\"/>" << std::endl;
      toFile << "\t\t\t" << "<greater to=\"" << accountNumber8 << "\"/>" << std::endl;
      toFile << "\t\t" << "</not>" << std::endl;
      toFile << "\t\t" << "<greater amount=\"" << amount2 << "\"/>" << std::endl;
      toFile << "\t" << "</query>" << std::endl;
    }
  }
  
}

int main(){
  int createCounter = 1;
  int counter = 1;
  // change i < n where (n-1) is the number of XML files created
  for(int i = 1; i < 5; i ++){
    std::vector<uint64_t> accounts;
    std::stringstream fileName;
    std::ofstream toFile;
    fileName << "Ltest" << i << ".xml";
    toFile.open(fileName.str());
    toFile << "<?xml version=""1.0"" encoding=""UTF-8""?>" << std::endl;
    reset(toFile);
    for(int j = 10000*(i-1)+1; j < 10000*i+1; j++){
      createRequest(accounts,toFile,j);
    }
    int chooseWhat = RNG(2);
    if(chooseWhat == 0){
      transferRequest(accounts,toFile,counter);
      counter = counter + 2;
    }
    else if(chooseWhat == 1){
      balanceRequest(accounts,toFile,counter);
      counter++;
    }
    else if(chooseWhat == 2){
      queryRequest(accounts,toFile,counter);
      counter = counter+4;
    } 
    toFile << "</transactions>" << std::endl;
    toFile.close();
  }

  // all XML files created at this point
  // XML files are named as "Ltestx.xml" where x ranges from 1 to (n-1) where n is max i from above for loop
  
  return 0;
}
