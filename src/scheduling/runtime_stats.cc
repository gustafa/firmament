#include "scheduling/runtime_stats.h"

namespace firmament {
RuntimeStats::RuntimeStats() { }

string RuntimeStats::ToJsonString(string name) {
  stringstream ss;
  ss << "\"" << name << "\": {\n";
  sort(arrival_times_.begin(), arrival_times_.end());
  sort(completion_times_.begin(), completion_times_.end());
  sort(missed_deadline_times_.begin(), missed_deadline_times_.end());

  ss << "\"arrival_times\": " << VectorToCSV(arrival_times_) << ",\n";
  ss << "\"completion_times\": " << VectorToCSV(completion_times_) << ",\n";
  ss << "\"missed_deadline_times_\": " << VectorToCSV(missed_deadline_times_) << "\n}\n";
  return ss.str();
}


string RuntimeStats::VectorToCSV(vector<uint64_t> &v) {
  if (!v.size()) {
    return "[]";
  }
  stringstream ss;
  ss << "[";
  uint64_t last_elem = v.size() - 1;
  for (uint64_t i = 0; i != last_elem; ++i) {
    ss << v[i] << ", ";
  }
  ss << v[last_elem];
  ss << "]";

  return ss.str();
}

} // namespace firmament
