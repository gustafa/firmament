#include "scheduling/runtime_stats.h"

#include "misc/map-util.h"

namespace firmament {
RuntimeStats::RuntimeStats():
  host_to_schedules_(new unordered_map<string, uint64_t>()) {

  }

string RuntimeStats::ToJsonString(string name) {
  stringstream ss;
  ss << "\"" << name << "\": {\n";
  // sort(arrival_times_.begin(), arrival_times_.end());
  // sort(completion_times_.begin(), completion_times_.end());
  // sort(missed_deadline_times_.begin(), missed_deadline_times_.end());
  ss << "\"arrival_completion\": [";
  if (!arrival_completions_.size()) {
    ss << "]\n";
  }
  for (auto it = arrival_completions_.begin(); it != arrival_completions_.end();) {
    ss << "(" << it->first << ", " << it->second.first << ", " << it->second.second << ")";

    ++it;
    if (it == arrival_completions_.end()) {
      ss << "]\n";
    }
    ss << ",";
  }



  // ss << "\"arrival_times\": " << VectorToCSV(arrival_times_) << ",\n";
  // ss << "\"completion_times\": " << VectorToCSV(completion_times_) << ",\n";
  ss << "\"missed_deadline_times_\": " << VectorToCSV(missed_deadline_times_);
  if (host_to_schedules_->size()) {
    ss << ",\n";
    for (auto it = host_to_schedules_->begin(); it != host_to_schedules_->end(); ++it) {
      ss << "\"" << it->first << "\": " << it->second;
    }
  }



  ss << "\n}\n";
  return ss.str();
}


void RuntimeStats::AddSample(const TaskDescriptor &td, const TaskFinalReport& report) {
  arrival_completions_.push_back(make_pair(td.uid(), make_pair(report.start_time(), report.finish_time())));
  if (td.has_absolute_deadline() && report.finish_time() > td.absolute_deadline()) {
      // The deadline is marked as missed at the point it expired.
      missed_deadline_times_.push_back(make_pair(td.uid(), td.absolute_deadline()));
    }
}


void RuntimeStats::AddScheduledStat(string hostname) {
  uint64_t *current = FindOrNull(*host_to_schedules_, hostname);

  if (current) {
    (*host_to_schedules_)[hostname] = *current +1;
  } else {
    (*host_to_schedules_)[hostname] = 1;
  }
}


string RuntimeStats::VectorToCSV(vector<pair<uint64_t, uint64_t>> &v) {
  if (!v.size()) {
    return "[]";
  }
  stringstream ss;
  ss << "[";
  uint64_t last_elem = v.size() - 1;
  for (uint64_t i = 0; i != last_elem; ++i) {
    ss << "(" << v[i].first  << ", " <<  v[i].second << "), ";
  }
  ss << "(" << v[last_elem].first << ", " << v[last_elem].second;
  ss << "]";

  return ss.str();
}

} // namespace firmament
