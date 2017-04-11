#include <iostream>
#include <pqxx/pqxx>
#include "parse.hpp"

using namespace pqxx;

#ifndef _DBRUN_
#define _DBRUN_

void addAccount (connection *C, std::vector<std::tuple<long long, double, std::string>> * parsedAccounts);
void balanceCheck (connection *C, std::vector<std::tuple<long long, std::string>> *parsedBalance);

void makeTransfers (connection *C, std::vector<Parse::Transfer> *parsedTransfer);

connection * dbRun (int reset);


#endif //_DBRUN_       
