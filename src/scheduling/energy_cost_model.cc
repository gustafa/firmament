// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Energy based cost model.

#include <string>

#include "scheduling/energy_cost_model.h"

namespace firmament {

EnergyCostModel::EnergyCostModel() { }

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
  return 0ULL;
}

Cost_t EnergyCostModel::ClusterAggToResourceNodeCost(ResourceID_t target) {
  return 0ULL;
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

}  // namespace firmament
