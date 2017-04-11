#ifndef COMBO_MT_HPP
#define COMBO_MT_HPP

#include <memory>
#include <string>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <functional>
#include <vector>


class ComboMT
{
public:
  ComboMT();
  ~ComboMT();
  int run();

private:
  static void serviceRequest(int client_connection_fd);
  static void readXBytes(int socket, unsigned int x, void* buffer);
  bool RUNNING;
  // void signalHandler(int signum);
  std::vector<std::thread> runningThreads;


};
#endif
