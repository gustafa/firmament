#ifndef FIRMAMENT_HAPROXY_CONTROLLER
#define FIRMAMENT_HAPROXY_CONTROLLER

#include "base/job_desc.pb.h"
#include "base/types.h"

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

  void GenerateJobs(vector<JobDescriptor*> &jobs, uint64_t number_of_jobs);

 private:
  string HAProxyCommand(string args);

  boost::shared_ptr<vector<string>> stats_headers;

  unordered_map<string, string> stats_map;


};

} // namespace firmament

#endif // FIRMAMENT_HAPROXY_CONTROLLER
