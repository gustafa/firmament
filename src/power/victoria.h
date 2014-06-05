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
  std::vector<pair<int, std::string>> host_port_vector_;

  const std::vector<double> *xs;
  const std::vector<double> *ys;

  StreamSocketsAdapter<BaseMessage>* m_adapter_;
  StreamSocketsChannel<BaseMessage>* chan_;

  std::string coordinator_uri;


  bool sendStats(BaseMessage *bm);

  double toRealWatts(double measuredWatt);

  void HandleWrite(const boost::system::error_code& error,
        size_t bytes_transferred);

  bool ConnectToCoordinator(const string& coordinator_uri);
};

#endif //VICTORIA_H
