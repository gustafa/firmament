// The Firmament project
// Copyright (c) 2013 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Implementation of the coordinator knowledge base.

#include "scheduling/knowledge_base.h"

#include <deque>

#include "misc/equivclasses.h"
#include "misc/map-util.h"
#include "misc/utils.h"

namespace firmament {



KnowledgeBase::KnowledgeBase() :
  webreqs_since_last_check_(0),
  webreq_window_first_(0),
  webreq_window_last_(0),
  application_stats_(new AppStatsMap_t()),
  runtime_stats_map_(new RuntimeStatsMap_t()) {
}

void KnowledgeBase::AddMachineSample(
    const MachinePerfStatisticsSample& sample) {
  ResourceID_t rid = ResourceIDFromString(sample.resource_id());
  // Check if we already have a record for this machine
  deque<MachinePerfStatisticsSample>* q =
      FindOrNull(machine_map_, rid);
  if (!q) {
    // Add a blank queue for this machine
    CHECK(InsertOrUpdate(&machine_map_, rid,
                         deque<MachinePerfStatisticsSample>()));
    q = FindOrNull(machine_map_, rid);
    CHECK_NOTNULL(q);
  }
  if (q->size() * sizeof(sample) >= MAX_SAMPLE_QUEUE_CAPACITY)
    q->pop_front();  // drom from the front
  q->push_back(sample);
}

void KnowledgeBase::AddTaskSample(const TaskPerfStatisticsSample& sample) {
  TaskID_t tid = sample.task_id();
  // Check if we already have a record for this task
  deque<TaskPerfStatisticsSample>* q = FindOrNull(task_map_, tid);
  if (!q) {
    // Add a blank queue for this task
    CHECK(InsertOrUpdate(&task_map_, tid,
                         deque<TaskPerfStatisticsSample>()));
    q = FindOrNull(task_map_, tid);
    CHECK_NOTNULL(q);
  }
  if (q->size() * sizeof(sample) >= MAX_SAMPLE_QUEUE_CAPACITY)
    q->pop_front();  // drop from the front
  q->push_back(sample);

  if (sample.has_nginx_stats()) {
    // TODO Waiting holds the total number of requests, silly but true!
    webreqs_since_last_check_ += sample.nginx_stats().waiting();
  }
}

uint64_t KnowledgeBase::GetAndResetWebreqs(uint64_t &num_seconds) {
  uint64_t web_reqs = webreqs_since_last_check_;
  num_seconds = ((webreq_window_last_ - webreq_window_first_) / 1000000) + 1;
  webreqs_since_last_check_ = 0;
  webreq_window_first_ = webreq_window_last_;

  return web_reqs;
}

const deque<MachinePerfStatisticsSample>* KnowledgeBase::GetStatsForMachine(
      ResourceID_t id) const {
  const deque<MachinePerfStatisticsSample>* res = FindOrNull(machine_map_, id);
  return res;
}

const deque<TaskPerfStatisticsSample>* KnowledgeBase::GetStatsForTask(
      TaskID_t id) const {
  const deque<TaskPerfStatisticsSample>* res = FindOrNull(task_map_, id);
  return res;
}

const deque<TaskFinalReport>* KnowledgeBase::GetFinalStatsForTask(
      TaskEquivClass_t id) const {
  const deque<TaskFinalReport>* res = FindOrNull(task_exec_reports_, id);
  return res;
}

void KnowledgeBase::DumpMachineStats(const ResourceID_t& res_id) const {
  // Sanity checks
  const deque<MachinePerfStatisticsSample>* q =
      FindOrNull(machine_map_, res_id);
  if (!q)
    return;
  // Dump
  LOG(INFO) << "STATS FOR " << res_id << ": ";
  LOG(INFO) << "Have " << q->size() << " samples.";
  for (deque<MachinePerfStatisticsSample>::const_iterator it = q->begin();
      it != q->end();
      ++it) {
    LOG(INFO) << it->free_ram();
  }
}
void KnowledgeBase::ProcessTaskFinalReport(const TaskDescriptor &td, const TaskFinalReport& report) {
  VLOG(2) << "Processing the final task report for task "  << td.uid();
  TaskID_t tid_equiv = report.task_id();
  CHECK(td.has_task_type());

  RuntimeStats *runtime_stats = FindPtrOrNull(*runtime_stats_map_, td.task_type());

  if (runtime_stats == NULL) {
    VLOG(2) << "Creating new runtime statistics for task_type " << td.task_type();
    runtime_stats = new RuntimeStats();
    (*runtime_stats_map_)[td.task_type()] = runtime_stats;
  }

  runtime_stats->AddArrivalTime(report.start_time());
  runtime_stats->AddCompletionTime(report.finish_time());
  if (td.has_absolute_deadline()) {
    if (report.finish_time() > td.absolute_deadline()) {
      // The deadline is marked as missed at the point it expired.
      runtime_stats->AddMissedDeadlineTime(td.absolute_deadline());
    }
  }

  // Check if we already have a record for this task
  deque<TaskFinalReport>* q = FindOrNull(task_exec_reports_, tid_equiv);
  if (!q) {
    // Add a blank queue for this task
    CHECK(InsertOrUpdate(&task_exec_reports_, tid_equiv,
                         deque<TaskFinalReport>()));
    q = FindOrNull(task_exec_reports_, tid_equiv);
    CHECK_NOTNULL(q);
  }
  if (q->size() * sizeof(report) >= MAX_SAMPLE_QUEUE_CAPACITY)
    q->pop_front();  // drop from the front
  q->push_back(report);


  VLOG(1) << "Recorded final report for task " << report.task_id();
}

string KnowledgeBase::GetRuntimesAsJson() {
  VLOG(2) << "Creating runtime stats string";
  stringstream ss;
  for (auto it = runtime_stats_map_->begin(); it != runtime_stats_map_->end();) {
    stringstream name;
    name << it->first;
    VLOG(2) << "Outputting stats for: " << name;
    ss << it->second->ToJsonString(name.str());
    ++it;
    if (it != runtime_stats_map_->end()) {
      ss << ",";
    }
  }
  return ss.str();
}

}  // namespace firmament
