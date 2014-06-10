#ifndef FIRMAMENT_HAPROXY_CONTROLLER
#define FIRMAMENT_HAPROXY_CONTROLLER

#include "base/job_desc.pb.h"
#include "base/types.h"

#include <boost/shared_ptr.hpp>
#include <unordered_map>
#include <unordered_set>

#include <vector>


namespace firmament {


class HAProxyController {

 public:
  HAProxyController(string server_backend);

  void ApplyDeltas();
  void GetStatistics();

  bool EnableServer(string hostname, uint64_t port);
  bool DisableServer(string hostname, uint64_t port);
  bool DisableServer(string webservername);


  void GenerateJobs(vector<JobDescriptor*> &jobs, uint64_t number_of_jobs, uint64_t size_per_job);


  inline uint64_t GetNumActiveJobs() { return num_active_jobs_;}
  inline void SetNumActiveJobs(uint64_t active_jobs) { num_active_jobs_ = active_jobs;}

  void DisableRandomServer(double load);

 private:
  string HAProxyCommand(string args);

  string server_backend_;
  boost::shared_ptr<vector<string>> stats_headers;
  unordered_map<string, string> stats_map;

  uint64_t current_web_job_;
  uint64_t num_active_jobs_;
  uint64_t start_port_;
  uint64_t num_ports_;


  unordered_set<string> running_servers_;
};

} // namespace firmament

#endif // FIRMAMENT_HAPROXY_CONTROLLER
