CC := gcc
CXX := g++
LD := g++
CXXFLAGS := -O3
LDFLAGS := -O3

CXXFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib

parse: parse.cpp
	$(LD) $(LDFLAGS) -g -Wall -pedantic -o parse parse.cpp -lxerces-c -lnsl -lpthread