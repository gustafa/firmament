// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// UNIX stream sockets-based messaging adapter.

#ifndef FIRMAMENT_PLATFORMS_UNIX_MESSAGING_STREAMSOCKETS_H
#define FIRMAMENT_PLATFORMS_UNIX_MESSAGING_STREAMSOCKETS_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

// This needs to go above other includes as it defines __PLATFORM_UNIX__
#include "platforms/unix/common.h"

#include "base/common.h"
#include "messages/base_message.pb.h"
#include "misc/messaging_interface.h"
#include "platforms/common.h"
#include "platforms/unix/tcp_connection.h"
#include "platforms/unix/async_tcp_server.h"
#include "platforms/unix/stream_sockets_channel.h"

namespace firmament {
namespace platform_unix {
namespace streamsockets {

class TCPConnection;  // forward declaration
class AsyncTCPServer;  // forward declaration
template <typename T>
class StreamSocketsChannel;  // forward declaration

// Messaging adapter.
template <typename T>
class StreamSocketsAdapter : public firmament::MessagingAdapterInterface<T>,
  public boost::enable_shared_from_this<StreamSocketsAdapter<T> >,
  private boost::noncopyable {
 public:
  StreamSocketsAdapter();
  virtual ~StreamSocketsAdapter();
  void AwaitNextMessage();
  void AddChannelForConnection(TCPConnection::connection_ptr connection);
  void CloseChannel(MessagingChannelInterface<T>* chan);
  bool EstablishChannel(const string& endpoint_uri,
                        MessagingChannelInterface<T>* chan);
  MessagingChannelInterface<T>* GetChannelForEndpoint(
      const string& endpoint);
  void ListenURI(const string& endpoint_uri);
  void Listen(const string& hostname, const string& port);
  string Listen(const string& hostname);
  bool ListenReady();
  void RegisterAsyncMessageReceiptCallback(
      typename AsyncMessageRecvHandler<T>::type callback);
  void RegisterAsyncErrorPathCallback(
      typename AsyncErrorPathHandler<T>::type callback);
  bool SendMessageToEndpoint(const string& endpoint_uri, T& message);
  void StopListen();
  virtual ostream& ToString(ostream* stream) const;

  size_t NumActiveChannels() {
    return endpoint_channel_map_.size();
  }
  void DumpActiveChannels() {
    LOG(INFO) << NumActiveChannels() << " active channels at "
              << *this;

    for (__typeof__(endpoint_channel_map_.begin()) chan_iter =
         endpoint_channel_map_.begin();
         chan_iter != endpoint_channel_map_.end();
         ++chan_iter) {
      LOG(INFO) << "[" << chan_iter->second << "]: Local: "
                << chan_iter->second->LocalEndpointString()
                << " Remote: " << chan_iter->second->RemoteEndpointString();
    }
  }

 private:
  void HandleAsyncMessageRecv(
      const boost::system::error_code& error, size_t bytes_transferred,
      StreamSocketsChannel<T>* chan);
  bool _EstablishChannel(const string& endpoint_uri,
                         StreamSocketsChannel<T>* chan);

  typename AsyncMessageRecvHandler<T>::type message_recv_handler_;
  typename AsyncErrorPathHandler<T>::type error_path_handler_;
  shared_ptr<AsyncTCPServer> tcp_server_;
  scoped_ptr<boost::thread> tcp_server_thread_;
  //set<shared_ptr<StreamSocketsChannel<T> > > active_channels_;
  map<const string, StreamSocketsChannel<T>*>
      endpoint_channel_map_;
  map<StreamSocketsChannel<T>*, Envelope<T>*>
      channel_recv_envelopes_;
  // Synchronization variables, locks tec.
  boost::mutex message_wait_mutex_;
  boost::condition_variable message_wait_condvar_;
  boost::mutex channel_recv_envelopes_mutex_;
  boost::mutex endpoint_channel_map_mutex_;
  bool message_wait_ready_;
};

}  // namespace streamsockets
}  // namespace platform_unix
}  // namespace firmament

#endif  // FIRMAMENT_PLATFORMS_UNIX_MESSAGING_STREAMSOCKETS_H
