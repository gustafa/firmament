#include "base/types.h"
#ifndef FIRMAMENT_SCHEDULING_COMPLETION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_COMPLETION_STATISTICS_H
namespace firmament {
class CompletionStatistics {
 public:
  CompletionStatistics();

  uint64_t completed;
  uint64_t preempted;
  uint64_t moved_to;
  uint64_t missed_deadlines;
  uint64_t made_deadlines;
};

} // namespace firmament
#endif // FIRMAMENT_SCHEDULING_COMPLETION_STATISTICS_H