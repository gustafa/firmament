#ifndef FIRMAMENT_HAPROXY_CONTROLLER
#define FIRMAMENT_HAPROXY_CONTROLLER

#include "base/job_desc.pb.h"

#include <boost/shared_ptr.hpp>
#include <unordered_map>

#include <vector>


namespace firmament {


class HAProxyController {

 public:
  HAProxyController();

  void ApplyDeltas();
  void GetStatistics();

  bool DisableServer(string hostname);

  bool EnableServer(string hostname);

  JobDescriptor *GetJobs(uint64_t next_seconds);

 private:
  string HAProxyCommand(string args);

  boost::shared_ptr<vector<string>> stats_headers;

  unordered_map<string, string> stats_map;


};

} // namespace firmament

#endif // FIRMAMENT_HAPROXY_CONTROLLER
