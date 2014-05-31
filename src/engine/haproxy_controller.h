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

  bool EnableServer(string hostname, uint64_t port);
  bool DisableServer(string hostname, uint64_t port);

  void GenerateJobs(vector<JobDescriptor*> &jobs, uint64_t number_of_jobs);

  inline uint64_t GetNumActiveJobs() { return num_active_jobs_;}
  inline void SetNumActiveJobs(uint64_t active_jobs) { num_active_jobs_ = active_jobs;}


 private:
  string HAProxyCommand(string args);

  boost::shared_ptr<vector<string>> stats_headers;

  unordered_map<string, string> stats_map;

  uint64_t current_web_job_;

  uint64_t num_active_jobs_;

  uint64_t start_port_;


};

} // namespace firmament

#endif // FIRMAMENT_HAPROXY_CONTROLLER
