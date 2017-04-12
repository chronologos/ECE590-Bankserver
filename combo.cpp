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
#include "db.h"
#include "utility.hpp"

using namespace std;
using namespace xercesc;

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.


int main(int argc, char *argv[]) {
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
  connection *C;
  while (true){
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      return -1;
    } //if

    //after accept, pthread create

    uint64_t length = 0;
    // char* buffer = 0;
    // we assume that sizeof(length) will return 4 here.
    ReadXBytes(client_connection_fd, sizeof(length), (void*)(&length));
    length = be64toh(length);
    // cout << length << endl;
    char *buffer = new char[length+1];
    memset(buffer, '\0', sizeof(char)*(length+1));
    // cout << buffer << endl;
    ReadXBytes(client_connection_fd, length, (void*)buffer);

    string s(buffer);
    // cout << buffer << endl;
    Parse parser;
    parser.readFile(s, true);
    delete[] buffer;

   //DB Set-up, if reset == True, drop all tables

    if (parser.reset == true){
      C = dbRun(1);
    }
    else {
      C = dbRun(0);
    }
    //DB insertion calls

    auto createResults = addAccount(C, &parser.creates);
    auto balanceResults = balanceCheck(C, &parser.balances);
    auto transferResults = makeTransfers(C, &parser.transfers);
    auto queriesResults = makeQueries(C, &parser.queries);

    // start constructing reply
    string reply = "";
    reply += "<?xml version='1.0' encoding='UTF-8'?>\r\n<results>\r\n";
    ostringstream resStream;
    for (auto &createResult : createResults){
      if (createResult.success && createResult.ref != "") {
        resStream << "<success " << "ref=\"" << createResult.ref << "\">" << "created</success>\r\n";
      }
      else if (createResult.success && createResult.ref == "") {
        resStream << "<success>created</success>\r\n";
      }
      else if (!createResult.success && createResult.ref != "") {
        resStream << "<error " << "ref=\"" << createResult.ref + "\">" << "not created</error>\r\n";
      }
      else if (!createResult.success && createResult.ref == "") {
        resStream << "<error>not created</error>\r\n";
      }
    }

    for (auto &balanceResult : balanceResults){
      if (balanceResult.success && balanceResult.ref != "") {
        resStream << "<success " << "ref=\"" << balanceResult.ref << "\">" << to_string(balanceResult.balance) << "</success>\r\n";
      }
      else if (balanceResult.success && balanceResult.ref == "") {
        resStream << "<success>" << to_string(balanceResult.balance) << "</success>\r\n";

      }
      else if (!balanceResult.success && balanceResult.ref != "") {
        resStream << "<error " << "ref=\"" << balanceResult.ref << "\">" << "balance not found</error>\r\n";
      }
      else if (!balanceResult.success && balanceResult.ref == "") {
        resStream << "<error>balance not found</error>\r\n";
      }
    }

    for (auto &transferResult : transferResults){
      if (transferResult.success && transferResult.ref != "") {
        resStream << "<success " << "ref=\"" << transferResult.ref << "\">" << "transferred</success>\r\n";
      }
      else if (transferResult.success && transferResult.ref == "") {
        resStream << "<success>transferred</success>\r\n";
      }
      else if (!transferResult.success && transferResult.ref != "") {
        resStream << "<error " << "ref=\"" << transferResult.ref << "\">" << "not transferred</error>\r\n";
      }
      else if (!transferResult.success && transferResult.ref == "") {
        resStream << "<error>not transferred</error>\r\n";
      }
    }
    reply += resStream.str();

    for (auto &queryResults : queriesResults){
      if (queryResults->ref != "") {
        reply += "  <results ref=\"" + queryResults->ref + "\">\r\n";
      } else {
        reply += "  <results>\r\n";
      }

      for (auto &queryResult : queryResults->results){
        reply += "    <transfer>\r\n";
        reply += "      <from>" + std::to_string(queryResult->from) +"</from>\r\n";
        reply += "      <to>" + std::to_string(queryResult->to) +"</to>\r\n";
        reply += "      <amount>" + std::to_string(queryResult->amount) +"</amount>\r\n";
        // TODO
        reply += "    </transfer>\r\n";
      }
      reply += "  </results>\r\n";
    }

    reply += "</results>";

    // cout << "OUR REPLY:" << endl;
    // cout << reply << endl;
    // cout << reply.size() << endl;
    WriteBytes(client_connection_fd, reply);
  }
  freeaddrinfo(host_info_list);
  close(socket_fd);
  //Close database connection
  C->disconnect();
  return 0;
}
