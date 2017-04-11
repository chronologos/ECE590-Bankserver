#include <iostream>
#include <pqxx/pqxx>
#include "parse.hpp"

using namespace pqxx;

#ifndef _DBRUN_
#define _DBRUN_

struct addResult{
  bool success;
  std::string ref;
  std::string errorMessage;
};

struct transferResult{
  bool success;
  std::string ref;
  std::string errorMessage;
};

struct balanceResult{
  bool success;
  double balance;
  std::string ref;
  std::string errorMessage;
};

std::vector<addResult> addAccount (connection *C, std::vector<std::shared_ptr<Parse::Create>> * parsedAccounts);
std::vector<balanceResult> balanceCheck (connection *C, std::vector<std::tuple<long long, std::string>> *parsedBalance);
std::vector<transferResult> makeTransfers (connection *C, std::vector<Parse::Transfer> *parsedTransfer);



connection * dbRun (int reset);


#endif //_DBRUN_
