CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq

parse: combo.cpp
	g++ -g -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o combo combo.cpp /usr/local/lib/libxerces-c.a -lpthread -lxerces-c -DMAIN_TEST $(EXTRAFLAGS)
