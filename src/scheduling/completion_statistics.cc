#include "scheduling/completion_statistics.h"

namespace firmament {

CompletionStatistics::CompletionStatistics() :
  completed(0),
  preempted(0),
  moved_to(0),
  missed_deadlines(0),
  made_deadlines(0) {
}

} // namespace firmament