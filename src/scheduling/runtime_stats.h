#include "base/types.h"
#ifndef FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
#define FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
namespace firmament {
class RuntimeStats {
 public:
  RuntimeStats();

  inline void AddArrivalTime(uint64_t t) {
    arrival_times_.push_back(t);
    VLOG(2) << "Adding arrival time";
  }


  inline void AddCompletionTime(uint64_t t) {
    completion_times_.push_back(t);
    VLOG(2) << "Adding completion time";
  }


  inline void AddMissedDeadlineTime(uint64_t t) {
    missed_deadline_times_.push_back(t);
    VLOG(2) << "Adding missed deadline";
  }

  string ToJsonString(string name);
  void AddScheduledStat(string hostname);



 private:
  vector<uint64_t> arrival_times_;
  vector<uint64_t> completion_times_;
  vector<uint64_t> missed_deadline_times_;

  shared_ptr<unordered_map<string, uint64_t>> host_to_schedules_;

  string VectorToCSV(vector<uint64_t> &v);
};

} // namespace firmament
#endif // FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H