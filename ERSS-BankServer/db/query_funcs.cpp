#include "query_funcs.h"

void add_account(connection *C, unsigned long long int accountNum, double balance) {
  string sql;
  
  sql = "INSERT INTO ACCOUNT (ACCOUNT_NUM,BALANCE)" \
    "VALUES (" + to_string(accountNum) + "," + to_string(balance) + ");";

   /* Create a transactional object. */
   work W(*C);

   /* Execute SQL query */
   W.exec( sql );
   W.commit();
}


void query1(connection *C, unsigned long long int accountNum, double balance, string tag) {

    string sql;
  
    sql = "SELECT * FROM account WHERE account_num = account_num";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "PlayerID TeamID UniformNum FirstName LastName MPG PPG RPG APG SPG BPG" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout <<c[0].as<string>()<<" "<<c[1].as<string>()<<" "<<c[2].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query2(connection *C, string team_color) {
  string sql;
  
  sql = "SELECT team.name FROM team, color WHERE color.color_id = team.color_id " \
    "AND color.name = '" + team_color + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "TeamName" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query3(connection *C, string team_name) {
  string sql;
  
  sql = "SELECT player.first_name, player.last_name FROM player, team WHERE player.team_id = "
    "team.team_id  AND team.name = '" + team_name + "' ORDER BY player.ppg DESC";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << " " << c[1].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query4(connection *C, string team_state, string team_color) {
  string sql;
  
  sql = "SELECT player.first_name, player.last_name, player.uniform_num FROM player, team, state, color "
    "WHERE player.team_id = team.team_id AND state.state_id = team.state_id  AND team.color_id = "
    "color.color_id AND state.name = '" + team_state + "' AND color.name = '" + team_color + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName JerseyNum" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << " " << c[1].as<string>() << " " << c[2].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query5(connection *C, int num_wins) {
   string sql;
  
  sql = "SELECT player.first_name, player.last_name, team.name, team.wins FROM player, team "
    "WHERE player.team_id = team.team_id AND team.wins > '" + to_string(num_wins) + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName TeamName TeamWins" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout<<c[0].as<string>()<<" "<<c[1].as<string>()<<" "<<c[2].as<string>()<<" "<<
      c[3].as<string>()<< endl;
  }
  //cout << "Operation done successfully" << endl;
}
