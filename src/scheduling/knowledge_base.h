// The Firmament project
// Copyright (c) 2013 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Coordinator knowledege base. This implements data structures and management
// code for the performance and utilization data reported by tasks and other
// coordinators.

#ifndef FIRMAMENT_ENGINE_KNOWLEDGE_BASE_H
#define FIRMAMENT_ENGINE_KNOWLEDGE_BASE_H

#include <string>
#include <map>
#include <deque>

#include "base/common.h"
#include "base/types.h"
#include "base/machine_perf_statistics_sample.pb.h"
#include "base/task_perf_statistics_sample.pb.h"
#include "base/task_final_report.pb.h"
#include "scheduling/application_statistics_interface.h"
#include "scheduling/runtime_stats.h"


namespace firmament {

typedef unordered_map<TaskDescriptor::TaskType, ApplicationStatistics*,
                      std::hash<int>> AppStatsMap_t;

typedef unordered_map<TaskDescriptor::TaskType, RuntimeStats*,
                      std::hash<int>> RuntimeStatsMap_t;
typedef unordered_map<string, uint64_t> ResourceCountMap_t;
// Limit stats queues to 1MB each
#define MAX_SAMPLE_QUEUE_CAPACITY 1024*1024

class KnowledgeBase {
 public:
  KnowledgeBase();
  void AddMachineSample(const MachinePerfStatisticsSample& sample);
  void AddTaskSample(const TaskPerfStatisticsSample& sample);
  void DumpMachineStats(const ResourceID_t& res_id) const;
  const deque<MachinePerfStatisticsSample>* GetStatsForMachine(
      ResourceID_t id) const;
  const deque<TaskPerfStatisticsSample>* GetStatsForTask(
      TaskID_t task_id) const;
  const deque<TaskFinalReport>* GetFinalStatsForTask(TaskID_t task_type) const; // TaskDescriptor::TaskType
  void ProcessTaskFinalReport(const TaskDescriptor &td, const TaskFinalReport& report);

  uint64_t GetAndResetWebreqs(uint64_t &num_seconds);

  double GetAndResetWebloads();

  inline  shared_ptr<AppStatsMap_t>  AppStats() {
    return application_stats_;
  }

  inline shared_ptr<vector<string>> GetRunningWebservers() {
    return running_webs_;
  }

  uint64_t NumRunningWebservers(string machine);
  void DeregisterWebserver(string machine);
  void RegisterWebserver(string machine, uint64_t port);
  string GetRuntimesAsJson();

  void AddScheduledTaskStat(TaskDescriptor::TaskType type, string hostname);



 protected:
  map<ResourceID_t, deque<MachinePerfStatisticsSample> > machine_map_;
  // TODO(malte): note that below sample queue has no awareness of time within a
  // task, i.e. it mixes samples from all phases
  map<TaskID_t, deque<TaskPerfStatisticsSample> > task_map_;
  map<TaskID_t, deque<TaskFinalReport> > task_exec_reports_;
  shared_ptr<AppStatsMap_t> application_stats_;

  double virtual_web_loads_;

  uint64_t num_web_loads_;

  shared_ptr<RuntimeStatsMap_t> runtime_stats_map_;

  shared_ptr<ResourceCountMap_t> running_webservers_;

  uint64_t webreqs_since_last_check_;

  uint64_t webreq_window_first_;
  uint64_t webreq_window_last_;

  shared_ptr<vector<string>> running_webs_;


};

}  // namespace firmament

#endif  // FIRMAMENT_ENGINE_KNOWLEDGE_BASE_H
