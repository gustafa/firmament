#include "base/types.h"

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