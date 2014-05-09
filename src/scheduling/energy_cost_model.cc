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
                  shared_ptr<TaskMap_t> task_map)
  : resource_map_(resource_map),
  job_map_(job_map),
  task_map_(task_map) {
 ResourceStatsMap *nginx_stats = new ResourceStatsMap();
  SetInitialNginxStats(nginx_stats);
  application_host_stats_["nginx"] = nginx_stats;
}

Cost_t EnergyCostModel::TaskToUnscheduledAggCost(TaskID_t task_id) {
  // TODO should be inversely proportional to deadline - estimated run-time
  // and > than cheapest place to schedule task.
  return 5ULL;
}

Cost_t EnergyCostModel::UnscheduledAggToSinkCost(JobID_t job_id) {
  return 0ULL;
}

Cost_t EnergyCostModel::TaskToClusterAggCost(TaskID_t task_id) {
  return 2ULL;
}

Cost_t EnergyCostModel::TaskToResourceNodeCost(TaskID_t task_id,
                                               ResourceID_t resource_id) {
  // Lookup the type of the task.
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  string app_name = (*td)->name();
  VLOG(2) << "APPNAME: " << app_name;

  VLOG(2) << "RESOURCE ID IS " << resource_id;
  ResourceStatsMap **application_stat = FindOrNull(application_host_stats_, app_name);

  if (application_stat) {
    ApplicationStatistics **application_host_stat = FindOrNull(**application_stat, resource_id);
    VLOG(2) << "FOUND APPLICATION STAT";

    if (application_stat) {
      VLOG(2) << "FOUND APP HOST STAT";
      return (uint64_t((*application_host_stat)->GetExpectedEnergyUse()));
    }
  }

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

void EnergyCostModel::SetInitialNginxStats(ResourceStatsMap *nginx_map) {
  // Dummy vars.
  (*nginx_map)[FindResourceID("titanic")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 5, 100);
  (*nginx_map)[FindResourceID("pandaboard")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 2, 80);
  (*nginx_map)[FindResourceID("michael")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 4, 300);
  (*nginx_map)[FindResourceID("gustafa")] = new ApplicationStatistics(ApplicationStatistics::REAL_TIME, 10, 300);

}

}  // namespace firmament5