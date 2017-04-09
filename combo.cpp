#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "parse.hpp"

using namespace std;
using namespace xercesc;

int main(int argc, char *argv[])
{
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = "44444";

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  socket_fd = socket(host_info_list->ai_family,
		     host_info_list->ai_socktype,
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  status = listen(socket_fd, 20);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if


  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  } //if

  //after accept, pthread create

  //while loop counter, parse number in the beginning
  char buffer[1024];

  uint64_t recSize = 1;

  int count = 0;

  std::string recData;
  
  //Keep recieving until buffer matches size of XML file
  while (count < recSize) {    
    int temp = recv(client_connection_fd, buffer, 1024, 0);
    
    if (recSize == 1) {
      count += (temp - 8);
      recData = buffer+8;
      recSize = *(uint64_t*)buffer;
    }
    else {
      count += temp;
      recData += buffer;
      cout << endl << "Still here" << endl;
    }
    //cout << endl << "recSize:" << recSize << endl;
  }

  Parse parser;
  parser.readFile(&recData);

//Parsing calls here
 
 
 //DB Set-up, if reset == True, drop all tables
 
 connection *C;
 
 if (parser.reset == TRUE){
    C = dbRun(1);
 }
 
 else {
    C = dbRun(0);
 }
     
//Send response back to the client
  std::string test = "Got your message"; //Test call, will be XML response
  send(client_connection_fd, test.c_str(), test.size(), 0);
 
  freeaddrinfo(host_info_list);
  close(socket_fd);
  
  //Close database connection
  C->disconnect();

  return 0;
}