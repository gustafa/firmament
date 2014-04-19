#ifndef VICTORIA_H
#define VICTORIA_H

#include <unordered_map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "messages/base_message.pb.h"
#include "misc/protobuf_envelope.h"
#include "platforms/unix/stream_sockets_adapter.h"
#include "platforms/unix/stream_sockets_adapter-inl.h"
#include "platforms/unix/stream_sockets_channel-inl.h"

using firmament::platform_unix::streamsockets::StreamSocketsAdapter;
using firmament::platform_unix::streamsockets::StreamSocketsChannel;
using firmament::BaseMessage;


class Victoria {

 public:
  Victoria(std::string firmament_master, std::vector<double> *xs, std::vector<double> *ys);
  void addMonitor(int port, std::string hostname);
  void run();

 private:
  std::unordered_map<int, std::string> portToHost;

  const std::vector<double> *xs;
  const std::vector<double> *ys;

  StreamSocketsAdapter<BaseMessage>* m_adapter_;
  StreamSocketsChannel<BaseMessage>* chan_;


  int sendStats(char *hostname, firmament::BaseMessage *base_message);

  double toRealWatts(double measuredWatt);
};

#endif //VICTORIA_H
