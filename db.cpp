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


std::vector<addResult> addAccount (connection *C, std::vector<Parse::Create> *parsedAccounts) {
  std::vector<addResult> res;
  for (auto it = parsedAccounts->begin(); it != parsedAccounts->end(); ++it) {
    //cout << "HEY" << endl;
    //cout << std::get<0>(*it) << endl;;
    addResult result = {}; //empty struct
    result.success = true;

    string sql;
    bool error = ((*it).error);
    if (error){
      result.success = false;
    }
    string account = to_string((*it).account);
    string balance = to_string((*it).balance);
    string ref = to_string((*it).ref);
    result.ref = ref;



    sql = "INSERT INTO ACCOUNTS (ACCOUNT_NUM,BALANCE)"			\
    "VALUES (" + account + "," + balance + ");";

    /* Create a transactional object. */
    work W(*C);

    /* Execute SQL query */
    W.exec( sql );
    W.commit();
    res.push_back(result);
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

    sql = "SELECT balance FROM accounts WHERE account_num = " +  accountNum;

    /* Create a non-transactional object. */
    nontransaction N(*C);

    /* Execute SQL query */
    result R( N.exec( sql ));

    /* List down all the records */
    //cout << "Balance" << endl;
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      //cout <<c[0].as<string>()<< endl << endl;
    }
    //cout << "Operation done successfully" << endl;
    res.push_back(balanceresult);
  }
  return res;
}

std::vector<transferResult> makeTransfers (connection *C, std::vector<Parse::Transfer> *parsedTransfer) {
  size_t vecSize = parsedTransfer->size();
  cout << "Transfer size:" << vecSize << endl;
  std::vector<transferResult> res;

  for (auto it = parsedTransfer->begin(); it != parsedTransfer->end(); ++it) {
    transferResult transferresult = {};
    string sql;
    string amount = to_string((*it).amount);
    string origin = to_string((*it).from);
    string destination = to_string((*it).to);

    int numTags = (*it).tags.size();

    if (numTags == 0) {
      sql = "INSERT INTO TRANSFERS (AMOUNT,ORIGIN,DESTINATION)"			\
      "VALUES (" + amount + "," + origin + "," + destination + ");";
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
  return res;
}

std::vector<queryResult> makeQueries (connection *C, std::vector<std::shared_ptr<Parse::Query>> *parsedQueries){
  std::vector<queryResult> res;
  cout << "processing queries" << endl;
  for (auto it = parsedQueries->begin(); it != parsedQueries->end(); ++it) {
    auto query = (*it);
    std::string sql = Parse::translateQuery(query);

    cout << sql << endl;
    /* Create a non-transactional object. */
    nontransaction N(*C);

    /* Execute SQL query */
    result R( N.exec(sql));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      cout <<c[0].as<string>()<< endl << endl;
    }
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
  }

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


  //Parse .txt files and insert
  //parseAccount(C);
  //exercise(C);



  return C;
}
