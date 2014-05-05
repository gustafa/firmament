#include "base/common.h"
#include "haproxy_controller.h"
#include "misc/map-util.h"
#include <boost/asio.hpp>
#include <stdio.h>


DEFINE_string(haproxy_conf_file, "/home/gjrh2/haproxy.conf",
              "The path of the nginx configuration file");

DEFINE_string(haproxy_socket_file, "/home/gjrh2/haproxy.socket",
              "The path of the HA proxy socket file used for "
              "stats extraction.");

using boost::asio::local::stream_protocol;

namespace firmament {


HAProxyController::HAProxyController() {
  stats_headers.reset(new vector<string>());
  //stats_map.reset(new unordered_map<string, unordered_map<string, string>>);
}


bool HAProxyController::DisableServer(string hostname) {
  string command = "disable server my_servers/" + hostname;
  HAProxyCommand(command);
  return true;
}

bool HAProxyController::EnableServer(string hostname) {

  string command = "enable server my_servers/" + hostname;
  HAProxyCommand(command);
  return true;
}


void HAProxyController::GetStatistics() {
  // TODO(gustafa):
  string stats_string = HAProxyCommand("show stat");
  const string outer_delimiter = "\n";
  const string inner_delimiter = ",";

  size_t pos = stats_string.find(outer_delimiter);

  string headers = stats_string.substr(0, pos);

  stats_string.erase(0, pos + 1);

  cout << "SIZE: " << stats_headers->size() << "\n";

  if (!(stats_headers->size())) {
    while ((pos = headers.find(inner_delimiter)) != string::npos) {
      stats_headers->push_back(headers.substr(0, pos));
      cout << "HEAD: "  << headers.substr(0, pos) << "\n";
      headers.erase(0, pos + 1);
    }

    cout << "SIZE: " << stats_headers->size() << "\n";
  }

  int64_t i;
  size_t inner_pos;

  while ((pos = stats_string.find(outer_delimiter)) != string::npos) {
    string stat_row = stats_string.substr(0, pos);
    stats_string.erase(0, pos + 1);



    // The two first columns are the inner key
    string inner_key;

    for (i = 0; i < 2; ++i) {
      inner_pos = stat_row.find(inner_delimiter);
      inner_key += stat_row.substr(0, inner_pos);
      stat_row.erase(0, inner_pos + 1);
    }

    if (inner_key.empty()) {
      continue;
    }

          cout << "INNER_KEY" << inner_key << "\n";

    while ((inner_pos = stat_row.find(inner_delimiter)) != string::npos) {

      // Iterate over each stat, look up its header and store it in the map.
      string temp =  stat_row.substr(0, inner_pos);

      string key = (*stats_headers)[i] + "_" + inner_key;

      stats_map[key] = temp;
      stat_row.erase(0, inner_pos + 1);
      ++i;
    }

    // for (auto keyval : stats_map) {
    //   cout << "STATS " << keyval.first << ": " << keyval.second << "\n";
    // }
  }

}

string HAProxyController::HAProxyCommand(string args) {
    std::stringstream ss;

    string command = string("haproxyctl ") + args;

    FILE *in;
    char buff[512];

    struct sockaddr_in serv_addr;
  if(!(in = popen(command.c_str(), "r"))){
    // ERROR
    }

  while(fgets(buff, sizeof(buff), in)!=NULL){
        ss << buff;
  }
  pclose(in);
  return ss.str();
}



JobDescriptor *GetJobs(uint64_t next_seconds) {

}


} // namespace firmament