#include <iostream>
#include <fstream>
#include <sstream>
#include <pqxx/pqxx>
#include <string>
#include "db.h"
//#include "exerciser.h"
//#include "query_funcs.h"

void parseAccount (connection *C) {
  string line, attr;
  int count = 0;
  vector<string> v;
  ifstream playerFile ("player.txt");
  if (playerFile.is_open()) {
    while (getline (playerFile, line)) {
      if (line.empty()) {
	continue;}
      stringstream ss(line);
      while (getline(ss, attr, ' ')) {
	v.push_back(attr);
      }
      count++;
    }
  }
  else {
    cout << "Unable to open file." << endl;
  }
  int i = 0;
  //int team_id = 0;
  for(i; i < count; i++) {
    int set = i * 11;
    int team_id = stoi(v[set+1]);
    int jersey_num = stoi(v[set+2]);
    string first_name = v[set+3];
    string last_name = v[set+4];
    int mpg = stoi(v[set+5]);
    int ppg = stoi(v[set+6]);
    int rpg = stoi(v[set+7]);
    int apg = stoi(v[set+8]);
    double spg = stod(v[set+9]);
    double bpg = stod(v[set+10]);
    add_player(C, team_id, jersey_num, first_name, last_name, mpg, ppg, rpg, apg, spg, bpg);
  }
  playerFile.close();
}

using namespace std;
using namespace pqxx;

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
    "TAG                        TEXT     NOT NULL);";
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
