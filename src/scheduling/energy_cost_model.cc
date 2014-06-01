// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Energy based cost model.

#include <string>

#include "misc/map-util.h"
#include "misc/utils.h"

#include "scheduling/energy_cost_model.h"


namespace firmament {

typedef TaskDescriptor::TaskType TaskType;

EnergyCostModel::EnergyCostModel(shared_ptr<ResourceMap_t> resource_map, shared_ptr<JobMap_t> job_map,
                  shared_ptr<TaskMap_t> task_map, shared_ptr<KnowledgeBase> knowledge_base,
                  shared_ptr<ResourceHostMap_t> resource_to_host)
  : resource_map_(resource_map),
  job_map_(job_map),
  task_map_(task_map),
  knowledge_base_(knowledge_base),
  resource_to_host_(resource_to_host) {
  application_stats_ = knowledge_base_->AppStats();
 // ResourceStatsMap *nginx_stats = new ResourceStatsMap();
 //  SetInitialNginxStats(nginx_stats);
 //  application_host_stats_["nginx"] = nginx_stats;
}

Cost_t EnergyCostModel::TaskToUnscheduledAggCost(TaskID_t task_id, FlowSchedulingPriorityType priority) {
  // TODO should be inversely proportional to deadline - estimated run-time
  // and > than cheapest place to schedule task.
  // Return 0LL.
  TaskDescriptor::TaskType application = GetTaskType(task_id);

  if (priority == PRIORITY_HIGH) {
    // If the priority is high we want to insert a very high cost related with not scheduling it.
    // Currently set as 1 billion. TODO(gustafa): Make this value a multiplier of the energy cost
    // at the worst machine.
    VLOG(2) << "Observing a high priority task, penalising unscheduled cost.";
    return 1000000000ULL;
  } else {
    ApplicationStatistics *app_stats = FindPtrOrNull(*application_stats_, application);


    // See if we have application statistics setup with observable values.
    if (app_stats && app_stats->HasStatistics()) {
      VLOG(2) << "Found application " << application << " setting unscheduled cost proportional "
              << "to the worst energy choice.";

      return app_stats->WorstEnergy(); // TODO multiplier?
    } else {
      VLOG(2) << "New application found. Setting a high unscheduled cost to promote scheduling.";

      // TODO handle case when no application is found. Potentially ensure it gets scheduled somewhere
      // so we can estimate deadlines?
      return 1000ULL;
    }
  }
}

Cost_t EnergyCostModel::UnscheduledAggToSinkCost(JobID_t job_id) {
  return 0ULL;
}

Cost_t EnergyCostModel::TaskToClusterAggCost(TaskID_t task_id) {
  return 2ULL;
}

Cost_t EnergyCostModel::TaskToResourceNodeCost(TaskID_t task_id,
                                               ResourceID_t resource_id) {
  TaskDescriptor::TaskType application = GetTaskType(task_id);
  string host = (*resource_to_host_)[resource_id];
  VLOG(2) << "Estimating cost of " << application  << " on "  << host;

  ApplicationStatistics *app_stats = FindPtrOrNull(*application_stats_, application);

  if (!app_stats) {
    VLOG(2) << "Did not find application " << application << " in the database. "
            << "Creating new.";
    app_stats = new ApplicationStatistics();
    // TODO setup the new, unseen program with defaults.
    (*application_stats_)[application] = app_stats;
    return 20ULL;

  } else {
    // Get the stat for this resource
    // TODO check if this is the resource the task is currently on and
    // apply a discount!

    TaskDescriptor *td = FindPtrOrNull(*task_map_, task_id);
    // _Should_ exist.
    CHECK_NOTNULL(td);

    // TODO handle case when we have started a task!
    double completed = 0;


    if (td->has_absolute_deadline() &&
      app_stats->GetRuntime(host, completed) > (td->absolute_deadline() - GetCurrentTimestamp())) {
      // We are not expected to make the deadline, let the scheduler know.
      return POOR_SCHEDULING_CHOICE;
    } else {
      return app_stats->GetEnergy(host, 0);
    }
  }
}

Cost_t EnergyCostModel::ClusterAggToResourceNodeCost(ResourceID_t target) {
  return 5ULL;
}

Cost_t EnergyCostModel::ResourceNodeToResourceNodeCost(
    ResourceID_t source,
    ResourceID_t destination) {
  return 0ULL;
}

Cost_t EnergyCostModel::LeafResourceNodeToSinkCost(ResourceID_t resource_id) {
  return 0ULL;
}

Cost_t EnergyCostModel::TaskContinuationCost(TaskID_t task_id) {
  return 0ULL;
}

Cost_t EnergyCostModel::TaskPreemptionCost(TaskID_t task_id) {
  return 0ULL;
}

void EnergyCostModel::SetInitialStats() {

  ApplicationStatistics *wc_stats = new ApplicationStatistics();
  (*application_stats_)[TaskDescriptor::MAPREDUCE_WC] = wc_stats;

  //wc_stats->SetEnergy("tcp:localhost:8088", 300);
  //wc_stats->SetRuntime("tcp:localhost:8088", 4000);

}

TaskType EnergyCostModel::GetTaskType(TaskID_t task_id) {
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  return (*td)->task_type();
}

}  // namespace firmament
