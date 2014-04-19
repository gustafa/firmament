#ifndef VICTORIA_H
#define VICTORIA_H

#include <unordered_map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "messages/base_message.pb.h"
#include "misc/protobuf_envelope.h"




class Victoria {

 public:
  Victoria(std::string firmament_master, std::vector<double> *xs, std::vector<double> *ys);
  void addMonitor(int port, std::string hostname);
  void run();

 private:
  std::unordered_map<int, std::string> portToHost;

  const std::vector<double> *xs;
  const std::vector<double> *ys;


  int sendStats(char *hostname, firmament::BaseMessage *base_message);

  double toRealWatts(double measuredWatt);
};

#endif //VICTORIA_H
