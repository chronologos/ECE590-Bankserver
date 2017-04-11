#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#include <iostream>
#include <csignal>
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
#include <thread>
#include <unistd.h>

#include "parse.hpp"
#include "comboMT.hpp"
#include "db.h"

using namespace std;
using namespace xercesc;


ComboMT::ComboMT() {
  RUNNING = true;
  runningThreads = std::vector<std::thread>();
}

ComboMT::~ComboMT() {}

// void ComboMT::signalHandler(int signum) {
//   cout << "Interrupt signal (" << signum << ") received.\n";
//   RUNNING = false;
//   cout << "waiting for all threads to finish" << endl;
//   for (auto t: runningThreads){
//     t.join();
//   }
//   exit(signum);
// }

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void ComboMT::readXBytes(int socket, unsigned int x, void* buffer){
  int bytesRead = 0;
  int result;
  while (bytesRead < x)
  {
    result = read(socket, buffer + bytesRead, x - bytesRead);
    if (result < 1 )
    {
      cout << "socket error" << endl;
    }
    else{
      bytesRead += result;
    }
  }
}

void ComboMT::serviceRequest(int client_connection_fd){
  uint64_t length = 0;
  readXBytes(client_connection_fd, sizeof(length), (void*)(&length));
  length = be64toh(length);
  cout << length << endl;
  char buffer[length+1] = {};
  memset(buffer, '\0', sizeof(char)*(length+1));
  readXBytes(client_connection_fd, length, (void*)buffer);

  // Then process the data as needed.

  string s(buffer);
  //jcout << buffer << endl;


  //jcout << s << endl;
  Parse parser;
  parser.readFile(s, true);
  cout << "=======DONE============" << endl;

  //Parsing calls here


  //DB Set-up, if reset == True, drop all tables

  // connection *C;
  //
  // if (parser.reset == true){
  //   C = dbRun(1);
  // }
  //
  // else {
  //   C = dbRun(0);
  // }
  //
  //
  // //DB insertion calls
  //
  // addAccount(C, &parser.creates);
  // balanceCheck(C, &parser.balances);
  //
  // //Send response back to the client
  // std::string test = "Got your message"; //Test call, will be XML response
  // send(client_connection_fd, test.c_str(), test.size(), 0);
  //
  //
  //
  // //Close database connection
  // C->disconnect();
}

int ComboMT::run(){
  // signal(SIGINT, signalHandler);
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = "12345";

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
    while (RUNNING){
      client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
      if (client_connection_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
      } //if
      runningThreads.push_back(std::thread(serviceRequest, client_connection_fd));
    }
    //after accept, pthread create
    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
  }
int main(int argc, char *argv[]) {
  ComboMT combomt;
  combomt.run();
}
