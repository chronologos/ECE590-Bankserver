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

struct queryResult{
  long long from;
  long long to;
  double amount;
  std::vector<std::string> tags;
};

struct queryResults{
  std::string ref;
  std::vector<std::shared_ptr<queryResult>> results;
};


std::vector<addResult> addAccount (connection *C, std::vector<Parse::Create> *parsedAccounts);
std::vector<balanceResult> balanceCheck (connection *C, std::vector<std::tuple<long long, std::string>> *parsedBalance);
std::vector<transferResult> makeTransfers (connection *C, std::vector<Parse::Transfer> *parsedTransfer);
std::vector<queryResult> makeQueries (connection *C, std::vector<std::shared_ptr<Parse::Query>> *parsedTransfer);



connection * dbRun (int reset);


#endif //_DBRUN_
