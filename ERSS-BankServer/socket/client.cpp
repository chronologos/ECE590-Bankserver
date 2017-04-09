#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
  //Read .txt file for XML contents
  fstream input("input.txt", fstream::in);
  string dataString;
  getline( input, dataString, '\0');

  //cout << dataString << endl;
  input.close();
  //
  
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = "127.0.0.1";
  const char *port     = "44444";

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

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

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if



  //Get size of dataString
  //double stringSize = sizeof(dataString);

  //cout << "File Size:"<< stringSize << endl;

  //Transform from string to char


  //Get size of XML file in uint 64

  uint64_t dataInt = dataString.size();

  cout << "dataInt:" << dataInt << endl;


  //convert to binary representation (8 bytes)
  char * toSendBuffer = NULL;

  toSendBuffer = (char*) malloc(dataInt+9);

  *(uint64_t*)toSendBuffer = dataInt;

  //Add 8bytes to front of XML
  strcpy(toSendBuffer+8,dataString.c_str());

  //Add null terminator to end
  toSendBuffer[8+dataInt] = '\0';

  
  printf("%s\n",toSendBuffer+8);
  printf("%u\n", *(uint64_t *)toSendBuffer); 

  send(socket_fd, toSendBuffer, dataString.size()+9, 0);
  char temp[1025] = {'0', };


  //Recieve response from server
  recv(socket_fd, temp, 1024, 0);
  temp[1024] = '\0';
  printf("%s\n", temp);

  
  freeaddrinfo(host_info_list);
  close(socket_fd);

  return 0;
}
