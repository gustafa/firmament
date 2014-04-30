// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Energy scheduling cost model.

#ifndef FIRMAMENT_SCHEDULING_ENERGY_COST_MODEL_H
#define FIRMAMENT_SCHEDULING_ENERGY_COST_MODEL_H

#include <string>
#include <unordered_map>

#include "base/common.h"
#include "base/types.h"
#include "scheduling/application_statistics.h"
#include "scheduling/flow_scheduling_cost_model_interface.h"


namespace firmament {

typedef uint64_t Cost_t;

class EnergyCostModel : public FlowSchedulingCostModelInterface {
 public:
  EnergyCostModel();

  // Costs pertaining to leaving tasks unscheduled
  Cost_t TaskToUnscheduledAggCost(TaskID_t task_id);
  Cost_t UnscheduledAggToSinkCost(JobID_t job_id);
  // Per-task costs (into the resource topology)
  Cost_t TaskToClusterAggCost(TaskID_t task_id);
  Cost_t TaskToResourceNodeCost(TaskID_t task_id,
                                ResourceID_t resource_id);
  // Costs within the resource topology
  Cost_t ClusterAggToResourceNodeCost(ResourceID_t target);
  Cost_t ResourceNodeToResourceNodeCost(ResourceID_t source,
                                        ResourceID_t destination);
  Cost_t LeafResourceNodeToSinkCost(ResourceID_t resource_id);
  // Costs pertaining to preemption (i.e. already running tasks)
  Cost_t TaskContinuationCost(TaskID_t task_id);
  Cost_t TaskPreemptionCost(TaskID_t task_id);

  static void SetInitialNginxStats(unordered_map<string, ApplicationStatistics*> *nginx_map);

 private:
  unordered_map<string, unordered_map<string, ApplicationStatistics*>*> application_host_stats_;
};

}  // namespace firmament

#endif  // FIRMAMENT_SCHEDULING_ENERGY_COST_MODEL_H