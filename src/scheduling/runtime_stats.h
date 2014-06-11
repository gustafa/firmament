
#ifndef FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
#define FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
#include "base/types.h"
#include "base/task_final_report.pb.h"

namespace firmament {
class RuntimeStats {
 public:
  RuntimeStats();


  string ToJsonString(string name);
  void AddSample(const TaskDescriptor &td, const TaskFinalReport& report);

  void AddScheduledStat(string hostname);

 private:
  vector<uint64_t> arrival_times_;
  vector<pair<TaskID_t, pair<uint64_t, uint64_t>>> arrival_completions_;
  vector<pair<TaskID_t, uint64_t>> missed_deadline_times_;

  shared_ptr<unordered_map<string, uint64_t>> host_to_schedules_;


  string VectorToCSV(vector<pair<uint64_t, uint64_t>> &v);
};

} // namespace firmament
#endif // FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H