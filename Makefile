CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq

all: parse

parse: combo.cpp parse.cpp db.cpp
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o combo combo.cpp parse.cpp db.cpp -lpthread -lxerces-c -DMAIN_TEST $(EXTRAFLAGS)

clean:
	rm -f *~ parse
