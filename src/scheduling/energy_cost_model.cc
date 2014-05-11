// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Energy based cost model.

#include <string>

#include "misc/map-util.h"
#include "misc/utils.h"

#include "scheduling/energy_cost_model.h"


namespace firmament {

EnergyCostModel::EnergyCostModel(shared_ptr<ResourceMap_t> resource_map, shared_ptr<JobMap_t> job_map,
                  shared_ptr<TaskMap_t> task_map, shared_ptr<ResourceHostMap_t> resource_to_host)
  : resource_map_(resource_map),
  job_map_(job_map),
  task_map_(task_map),
  resource_to_host_(resource_to_host) {
 // ResourceStatsMap *nginx_stats = new ResourceStatsMap();
 //  SetInitialNginxStats(nginx_stats);
 //  application_host_stats_["nginx"] = nginx_stats;
}

Cost_t EnergyCostModel::TaskToUnscheduledAggCost(TaskID_t task_id, FlowSchedulingPriorityType priority) {
  // TODO should be inversely proportional to deadline - estimated run-time
  // and > than cheapest place to schedule task.

  if (priority == PRIORITY_HIGH) {
    // If the priority is high we want to insert a very high cost related with not scheduling it.
    // Currently set as 1 billion. TODO(gustafa): Make this value a multiplier of the energy cost
    // at the worst machine.
    return 1000000000ULL;
  } else {
    return 5ULL;
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

  string application = GetTaskApp(task_id);

  string host = (*resource_to_host_)[resource_id];

  VLOG(2) << "Estimating cost of " << application " on "  << host;


  // VLOG(2) << "RESOURCE ID IS " << resource_id;
  // ResourceStatsMap **application_stat = FindOrNull(application_host_stats_, app_name);

  // if (application_stat) {
  //   ApplicationStatistics **application_host_stat = FindOrNull(**application_stat, resource_id);
  //   VLOG(2) << "FOUND APPLICATION STAT";

  //   if (application_host_stat) {
  //     VLOG(2) << "FOUND APP HOST STAT";

  //     // Check if we estimate we'll be able to satisfy the deadline set.
  //     if ((*td)->has_absolute_deadline()) {
  //       uint64_t expected_completion = GetCurrentTimestamp() + (*application_host_stat)->GetExpectedRuntime();

  //       // Return this being a poor scheduling decision if we are predicted to miss the deadline.
  //       if (expected_completion >(*td)->absolute_deadline()) {
  //         return POOR_SCHEDULING_CHOICE;
  //       }
  //     }

  //     return (uint64_t((*application_host_stat)->GetExpectedEnergyUse()));
  //   }
  // }

  return 0ULL;
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
  application_stats_["wordcount"] = wc_stats;

  wc_stats->SetEnergy("tcp:localhost:8088", 300);
  wc_stats->SetRuntime("tcp:localhost:8088", 4000);

  // // Dummy vars. For real-time applications set the time it takes to the frequency of rescheduling

  // (*nginx_map)[FindResourceID("titanic")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 5, 5);
  // (*nginx_map)[FindResourceID("pandaboard")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 2, 5);
  // (*nginx_map)[FindResourceID("michael")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 4, 5);

  // VLOG(2) << "RESOURCE ID GUSTAFA: " << FindResourceID("gustafa");
  // (*nginx_map)[FindResourceID("gustafa")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 10, 5);

}

string EnergyCostModel::GetTaskApp(TaskID_t task_id) {
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  return (*td)->name();
}

}  // namespace firmament5