#include <iostream>
#include <pqxx/pqxx>
#include <string>

using namespace std;
using namespace pqxx;

#ifndef _QUERY_FUNCS_
#define _QUERY_FUNCS_

void add_account(connection *C, unsigned long long int accountNum, double balance);
void query1(connection *C, 
            int use_mpg, int min_mpg, int max_mpg,
	    int use_ppg, int min_ppg, int max_ppg,
	    int use_rpg, int min_rpg, int max_rpg,
	    int use_apg, int min_apg, int max_apg,
	    int use_spg, double min_spg, double max_spg,
	    int use_bpg, double min_bpg, double max_bpg
	    );

void query2(connection *C, string team_color);

void query3(connection *C, string team_name);

void query4(connection *C, string team_state, string team_color);

void query5(connection *C, int num_wins);

#endif //_QUERY_FUNCS_
