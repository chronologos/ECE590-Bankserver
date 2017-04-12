#include <iostream>
#include <fstream>
#include <sstream>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include "db.h"

//#include "exerciser.h"
//#include "query_funcs.h"

using namespace std;
using namespace pqxx;

// function to get string array from string representation
vector<string> getArray(string pString) {
  vector<string> tags;
  pString += ',';
  size_t begin = pString.find('{') + 1;
  size_t end = pString.find('}');
  size_t actualPosition;
  string tag;

  while(true) {
    actualPosition = pString.find(',', begin);
    if(actualPosition > end) {
      tag = pString.substr(begin, end - begin);
      tags.push_back(tag);
      return tags;
    }
    tag = pString.substr(end, actualPosition - begin);
    tags.push_back(tag);
    begin = actualPosition + 1;
  }
}

std::vector<addResult> addAccount (connection *C, std::vector<Parse::Create> *parsedAccounts) {
  std::vector<addResult> res;
  for (auto it = parsedAccounts->begin(); it != parsedAccounts->end(); ++it) {
    //cout << "HEY" << endl;
    //cout << std::get<0>(*it) << endl;;
    addResult structResult = {}; //empty struct
    structResult.success = true;

    string sql;
    bool error = ((*it).error);
    if (error){
      structResult.success = false;
    }
    string account = to_string((*it).account);
    string balance = to_string((*it).balance);
    string ref = to_string((*it).ref);
    structResult.ref = ref;

    //Error checking
    string err = "SELECT EXISTS (SELECT TRUE FROM accounts WHERE account_num=" + account+ ");";
    string status;

    nontransaction E(*C);
    result R( E.exec( err ));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      //cout <<c[0].as<string>()<< endl << endl;
      status=c[0].as<string>();
    }

    if (status == "t") {
      E.commit();
      structResult.success = false;
      res.push_back(structResult);
    }
    else {
      E.commit();
       sql = "INSERT INTO ACCOUNTS (ACCOUNT_NUM,BALANCE)"	\
	"VALUES (" + account + "," + balance + ");";

      work W(*C);

      try {
	/* Execute SQL query */
	W.exec( sql );
	W.commit();
	}
	catch (...) {
	}
      res.push_back(structResult);
    }
  }
  return res;
}

std::vector<balanceResult> balanceCheck (connection *C, vector<std::tuple<long long, string>> *parsedBalance) {
  std::vector<balanceResult> res;
  for (auto it = parsedBalance->begin(); it != parsedBalance->end(); ++it) {
    //cout << "HEY" << endl;
    //cout << std::get<0>(*it) << endl;
    balanceResult balanceresult = {};
    balanceresult.success = true;

    string sql;
    string accountNum = to_string(std::get<0>(*it));
    balanceresult.ref = std::get<1>(*it);

    //Error checking
    string err = "SELECT EXISTS (SELECT TRUE FROM accounts WHERE account_num=" + accountNum+ ");";
    string status;

    nontransaction E(*C);
    result R( E.exec( err ));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      status=c[0].as<string>();
    }

    if (status == "f") {
      E.commit();
      balanceresult.success = false;
      res.push_back(balanceresult);
    }
    else {
      E.commit();

      sql = "SELECT balance FROM accounts WHERE account_num = " +  accountNum;

      /* Create a non-transactional object. */
      nontransaction N(*C);

      /* Execute SQL query */
      result R( N.exec( sql ));

      /* List down all the records */
      cout << "Balance" << endl;
      for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
	cout <<c[0].as<string>()<< endl << endl;
	balanceresult.balance=c[0].as<double>();

      }
      //cout << "Operation done successfully" << endl;
      res.push_back(balanceresult);
    }
  }
  return res;
}

vector<transferResult> makeTransfers (connection *C, std::vector<Parse::Transfer> *parsedTransfer) {
  size_t vecSize = parsedTransfer->size();
  cout << "Transfer size:" << vecSize << endl;
  std::vector<transferResult> res;

  for (auto it = parsedTransfer->begin(); it != parsedTransfer->end(); ++it) {
    transferResult transferresult = {};
    transferresult.success = true;
    transferresult.ref = (*it).ref;

    string sql;
    string amount = to_string((*it).amount);
    string origin = to_string((*it).from);
    string destination = to_string((*it).to);

    int numTags = (*it).tags.size();

    //Error checking
    int skip = 0;
    if ( skip == 0){
      string err1 = "SELECT EXISTS (SELECT TRUE FROM accounts WHERE account_num=" + origin+ ");";
      string status1;

      nontransaction E1(*C);
      result R1( E1.exec( err1 ));
      for (result::const_iterator c = R1.begin(); c != R1.end(); ++c) {
	status1=c[0].as<string>();
      }
      if (status1 == "f") {
	transferresult.success = false;
	res.push_back(transferresult);
	skip = 1;
      }
      E1.commit();
    }
    if (skip != 1) {
      string err2 = "SELECT EXISTS (SELECT TRUE FROM accounts WHERE account_num=" + destination+ ");";
      string status2;

      nontransaction E2(*C);
      result R2( E2.exec( err2 ));
      for (result::const_iterator c = R2.begin(); c != R2.end(); ++c) {
	status2=c[0].as<string>();
      }

      if (status2 == "f") {
	transferresult.success = false;
	res.push_back(transferresult);
	skip = 1;
      }
      E2.commit();
    }

    //check if balance is negative
    if (skip != 1) {
      string err3 = "SELECT balance FROM accounts WHERE account_num = " +  origin;

      nontransaction E3(*C);
      result R3( E3.exec( err3 ));

      /* List down all the records */
      for (result::const_iterator c = R3.begin(); c != R3.end(); ++c) {
	//cout <<c[0].as<string>()<< endl << endl;
	double accBalance = c[0].as<double>();
	if ((accBalance < 0) || (accBalance < (*it).amount) ) {
	  transferresult.success = false;
	  res.push_back(transferresult);
	  skip = 1;
	}
      }
      E3.commit();
    }

    if (skip != 1) {

      if (numTags == 0) {
	string tag = "blank";
	sql = "INSERT INTO TRANSFERS (AMOUNT,ORIGIN,DESTINATION, TAG)"	\
	  "VALUES (" + amount + "," + origin + "," + destination + ",'{" + tag + "}');";
	work W(*C);
	W.exec( sql );
	W.commit();
      }
      else {
	string tag = (*it).tags[0];
	sql = "INSERT INTO TRANSFERS (AMOUNT,ORIGIN,DESTINATION,TAG)"	\
	  "VALUES (" + amount + "," + origin + "," + destination + ",'{" + tag + "}');";

	work W(*C);
	W.exec( sql );
	W.commit();

	string add;
	for(int i = 1; i < numTags; i++) {
	  string otherTags = (*it).tags[i];
	  cout << otherTags << endl;
	  //add = "UPDATE transfers SET tag = array_cat(tag, '{" +otherTags + "}');";
	  add = "UPDATE transfers SET tag = tag || '{" + otherTags + "}';";
	  work A(*C);
	  A.exec( add );
	  A.commit();
	}
      }

      string transFrom = "UPDATE accounts SET balance=balance - " + amount +
	" WHERE account_num = " + origin + ";";
      work F(*C);
      F.exec( transFrom );
      F.commit();

      string transTo = "UPDATE accounts SET balance=balance + " + amount +
	" WHERE account_num = " + destination + ";";
      work T(*C);
      T.exec( transTo );
      T.commit();
      res.push_back(transferresult);
    }
}

  return res;
}

vector<shared_ptr<queryResults>> makeQueries (connection *C, std::vector<std::shared_ptr<Parse::Query>> *parsedQueries){
  vector<shared_ptr<queryResults>> res; //empty vector for results of ALL queries
  cout << "processing queries" << endl;
  for (auto it = parsedQueries->begin(); it != parsedQueries->end(); ++it) {
    std::shared_ptr<queryResults> qResultsPtr(new queryResults()); //empty struct for results of ONE query
    qResultsPtr->ref = (*it)->ref;
    auto query = (*it);
    std::string sql = Parse::translateQuery(query);

    cout << sql << endl;
    /* Create a non-transactional object. */
    nontransaction N(*C);
    /* Execute SQL query */
    result R( N.exec(sql));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      std::shared_ptr<queryResult> qResultPtr(new queryResult()); //empty struct for one result of ONE query

      int dcount = 0;
      for (auto d : c){
        // cout <<d.as<string>()<< endl;
        if (dcount == 1){
          pqxx::from_string<double>(d, (qResultPtr->amount));
          cout << qResultPtr->amount << endl;
        }
        else if (dcount == 2){
          pqxx::from_string<long long>(d, (qResultPtr->from));
          cout << qResultPtr->from << endl;

        }
        else if (dcount == 3){
          pqxx::from_string<long long>(d, (qResultPtr->to));
          cout << qResultPtr->to << endl;

        }
        else if (dcount == 4){
          // cout << (queryResultPtr->tags) << endl;
          qResultPtr->tags = getArray(d.as<string>());
          // d.find("is");
          // (queryResultPtr->tags).push_back(tag);
        }
        dcount++;
      }
      qResultsPtr->results.push_back(qResultPtr);
    }
    res.push_back(qResultsPtr);
  }
  return res;
}

connection * dbRun (int reset) {
  //Allocate & initialize a Postgres connection object
  connection *C;

  string aString;
  string tString;

  try{
    //Establish a connection to the database
    //Parameters: database name, user name, user password
    C = new connection("dbname=bank user=postgres password=passw0rd");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      //return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    //return 1;
  }

  //TODO: create ACCOUNT and TRANSFERS in the BANK database
  //      load table with rows from the provided source txt file

  //DROP DB Tables IF reset=true
  if (reset == 1) {
    //create transactional object
    work aD(*C);
    aString = "DROP TABLE IF EXISTS accounts";
    //Execute SQL query
    aD.exec(aString);
    aD.commit();


    work tD(*C);
    tString = "DROP TABLE IF EXISTS transfers";
    //Execute SQL query
    tD.exec(tString);
    tD.commit();

    cout << "Table successfully dropped." << endl;


  // Create Tables
  aString = "CREATE TABLE ACCOUNTS("		      \
  "ACCOUNT_NUM BIGINT PRIMARY KEY  NOT NULL,"	      \
  "BALANCE                 FLOAT   NOT NULL);";

  tString = "CREATE TABLE TRANSFERS("
  "TRANSFER_ID SERIAL PRIMARY KEY      NOT NULL,"
  "AMOUNT                     FLOAT    NOT NULL,"
  "ORIGIN                     BIGINT   NOT NULL,"
  "DESTINATION                BIGINT   NOT NULL,"
  "TAG                        TEXT[]   NOT NULL);";
  //Create transactional objects
  work aW(*C);
  aW.exec( aString );
  aW.commit();

  work tW(*C);
  tW.exec( tString );
  tW.commit();

  cout << "Table created successfully" << endl;


  }

  //Parse .txt files and insert
  //parseAccount(C);
  //exercise(C);



  return C;
}
