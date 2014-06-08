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
#include "scheduling/knowledge_base.h"
#include "misc/utils.h"
#include "scheduling/batchapp_statistics.h"
#include "scheduling/serviceapp_statistics.h"
#include "scheduling/flow_scheduling_cost_model_interface.h"


namespace firmament {




typedef int64_t Cost_t;

typedef unordered_map<ResourceID_t, ApplicationStatistics*, boost::hash<boost::uuids::uuid>> ResourceStatsMap;

class EnergyCostModel : public FlowSchedulingCostModelInterface {
 public:
  EnergyCostModel(shared_ptr<ResourceMap_t> resource_map, shared_ptr<JobMap_t> job_map,
                  shared_ptr<TaskMap_t> task_map, shared_ptr<KnowledgeBase> knowledge_base,
                  shared_ptr<ResourceHostMap_t> resource_to_host,
                  map<TaskID_t, ResourceID_t> *task_bindings);

  // Costs pertaining to leaving tasks unscheduled
  Cost_t TaskToUnscheduledAggCost(TaskID_t task_id, FlowSchedulingPriorityType priority);
  Cost_t UnscheduledAggToSinkCost(JobID_t job_id);
  // Per-task costs (into the resource topology)
  Cost_t TaskToClusterAggCost(TaskID_t task_id);
  Cost_t TaskToResourceNodeCost(TaskID_t task_id,
                                ResourceID_t resource_id);

  Cost_t TaskToResourceNodeCosts(TaskID_t task_id, const vector<ResourceID_t> &machine_ids,
                                 vector<Cost_t> &machine_task_costs);
  // Costs within the resource topology
  Cost_t ClusterAggToResourceNodeCost(ResourceID_t target);
  Cost_t ResourceNodeToResourceNodeCost(ResourceID_t source,
                                        ResourceID_t destination);
  Cost_t LeafResourceNodeToSinkCost(ResourceID_t resource_id);
  // Costs pertaining to preemption (i.e. already running tasks)
  Cost_t TaskContinuationCost(TaskID_t task_id);
  Cost_t TaskPreemptionCost(TaskID_t task_id);


  TaskDescriptor::TaskType GetTaskType(TaskID_t task_id);

  void SetInitialStats();


 private:

  // Lookup maps for various resources from the scheduler.
  shared_ptr<ResourceMap_t> resource_map_;

  // Information regarding tasks.
  shared_ptr<KnowledgeBase> knowledge_base_;
  shared_ptr<JobMap_t> job_map_;
  shared_ptr<TaskMap_t> task_map_;
  shared_ptr<ResourceHostMap_t> resource_to_host_;
  shared_ptr<AppStatsMap_t> application_stats_;

  map<TaskID_t, ResourceID_t> *task_bindings_;

  Cost_t BatchTaskToResourceNodeCosts(TaskID_t task_id, TaskDescriptor *td, const vector<ResourceID_t> &machine_ids,
                                                       vector<Cost_t> &machine_task_costs);

  Cost_t ServiceTaskToResourceNodeCosts(TaskID_t task_id, TaskDescriptor *td, const vector<ResourceID_t> &machine_ids,
                                      vector<Cost_t> &machine_task_costs);
};

}  // namespace firmament

#endif  // FIRMAMENT_SCHEDULING_ENERGY_COST_MODEL_H