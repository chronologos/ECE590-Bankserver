#include <iostream>
#include <pqxx/pqxx>

using namespace pqxx;

#ifndef _DBRUN_
#define _DBRUN_

connection * dbRun (int reset);


#endif //_DBRUN_       
