CC = g++

all: client server

client: client.cpp
	$(CC) client.cpp -o client

server: server.cpp
	$(CC) server.cpp -o server

clean:
	rm -f *~ server client
