parse: parse.cpp
	g++ -g -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o parse parse.cpp /usr/local/lib/libxerces-c.a -lpthread -lxerces-c -DMAIN_TEST
