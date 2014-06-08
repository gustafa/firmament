#include "base/types.h"
#ifndef FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
#define FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H
namespace firmament {
class RuntimeStats {
 public:
  RuntimeStats();

  inline void AddArrivalTime(uint64_t t) {
    arrival_times_.push_back(t);
  }


  inline void AddCompletionTime(uint64_t t) {
    completion_times_.push_back(t);
  }


  inline void AddMissedDeadlineTime(uint64_t t) {
    missed_deadline_times_.push_back(t);
  }

 private:
  vector<uint64_t> arrival_times_;
  vector<uint64_t> completion_times_;
  vector<uint64_t> missed_deadline_times_;
};

} // namespace firmament
#endif // FIRMAMENT_SCHEDULING_RUNTIME_STATISTICS_H