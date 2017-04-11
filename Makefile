CC=g++
CFLAGS=-std=c++11 -g
EXTRAFLAGS=-lpqxx -lpq -ltcmalloc

all: combo

combo: combo.cpp parse.cpp db.cpp utility.cpp
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o combo combo.cpp parse.cpp db.cpp utility.cpp -lpthread -lxerces-c -DMAIN_TEST $(EXTRAFLAGS)

comboMT: comboMT.cpp parse.cpp db.cpp utility.cpp
	$(CC) $(CFLAGS) -Wall -pedantic -L/usr/local/lib -I/usr/local/include -o comboMT comboMT.cpp parse.cpp db.cpp utility.cpp -lpthread -lxerces-c -DMAIN_TEST $(EXTRAFLAGS)

clean:
	rm -f *~ combo
