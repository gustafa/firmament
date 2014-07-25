// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Abstract class representing the interface for cost model implementations.

#ifndef FIRMAMENT_SCHEDULING_FLOW_SCHEDULING_COST_MODEL_H
#define FIRMAMENT_SCHEDULING_FLOW_SCHEDULING_COST_MODEL_H

#include <limits>
#include <string>

#include "base/common.h"
#include "base/types.h"

namespace firmament {

typedef int64_t Cost_t;

// List of cost models supported
enum FlowSchedulingCostModelType {
  COST_MODEL_TRIVIAL = 0,
  COST_MODEL_QUINCY = 1,
  COST_MODEL_ENERGY = 2,
  COST_MODEL_PERFORMANCE = 3,
};

// List of priorities supported
enum FlowSchedulingPriorityType {
  PRIORITY_LOW = 0,
  PRIORITY_MEDIUM = 1,
  PRIORITY_HIGH = 2,
};

class FlowSchedulingCostModelInterface {
 public:
  FlowSchedulingCostModelInterface() {};

  const Cost_t POOR_SCHEDULING_CHOICE = std::numeric_limits<int32_t>::max() - 10;

  virtual ~FlowSchedulingCostModelInterface() {};

  // Costs pertaining to leaving tasks unscheduled
  virtual Cost_t TaskToUnscheduledAggCost(TaskID_t task_id, FlowSchedulingPriorityType priority) = 0;
  virtual Cost_t UnscheduledAggToSinkCost(JobID_t job_id) = 0;
  // Per-task costs (into the resource topology)
  virtual Cost_t TaskToClusterAggCost(TaskID_t task_id) = 0;
  virtual Cost_t TaskToResourceNodeCost(TaskID_t task_id,
                                        ResourceID_t resource_id) = 0;
  // Populates a vector of cost for the given machine ids and returns the unscheduled cost.
  virtual Cost_t TaskToResourceNodeCosts(TaskID_t task_id, const vector<ResourceID_t> &machine_ids,  vector<Cost_t> &machine_task_costs) = 0;
  // Costs within the resource topology
  virtual Cost_t ClusterAggToResourceNodeCost(ResourceID_t target) = 0;
  virtual Cost_t ResourceNodeToResourceNodeCost(ResourceID_t source,
                                                ResourceID_t destination) = 0;
  virtual Cost_t LeafResourceNodeToSinkCost(ResourceID_t resource_id) = 0;
  // Costs pertaining to preemption (i.e. already running tasks)
  virtual Cost_t TaskContinuationCost(TaskID_t task_id) = 0;
  virtual Cost_t TaskPreemptionCost(TaskID_t task_id) = 0;

  const Cost_t MULTIPLIER_ = 1000;
};

}  // namespace firmament

#endif  // FIRMAMENT_SCHEDULING_FLOW_SCHEDULING_COST_MODEL_H
