// The Firmament project
// Copyright (c) 2012-2013 Ionel Gog <ionel.gog@cl.cam.ac.uk>
//
// Quincy scheduler.

#ifndef FIRMAMENT_ENGINE_ENERGY_SCHEDULER_H
#define FIRMAMENT_ENGINE_ENERGY_SCHEDULER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/common.h"
#include "base/types.h"
#include "base/job_desc.pb.h"
#include "base/task_desc.pb.h"
#include "engine/executor_interface.h"
#include "engine/haproxy_controller.h"
#include "scheduling/dimacs_exporter.h"
#include "scheduling/event_driven_scheduler.h"
#include "scheduling/flow_graph.h"
#include "scheduling/flow_node_type.pb.h"
#include "scheduling/scheduling_delta.pb.h"
#include "scheduling/scheduling_parameters.pb.h"
#include "storage/reference_interface.h"

namespace firmament {
namespace scheduler {

using executor::ExecutorInterface;

class EnergyScheduler : public EventDrivenScheduler {
 public:
  EnergyScheduler(shared_ptr<JobMap_t> job_map,
                  shared_ptr<ResourceMap_t> resource_map,
                  shared_ptr<store::ObjectStoreInterface> object_store,
                  shared_ptr<TaskMap_t> task_map,
                  shared_ptr<TopologyManager> topo_mgr,
                  MessagingAdapterInterface<BaseMessage>* m_adapter,
                  ResourceID_t coordinator_res_id,
                  const string& coordinator_uri,
                  const SchedulingParameters& params);
  ~EnergyScheduler();
  void HandleTaskCompletion(TaskDescriptor* td_ptr,
                            TaskFinalReport* report);
  uint64_t ScheduleJob(JobDescriptor* job_desc);
  virtual ostream& ToString(ostream* stream) const {
    return *stream << "<EnergyScheduler, parameters: "
                   << parameters_.DebugString() << ">";
  }

 protected:
  const ResourceID_t* FindResourceForTask(TaskDescriptor* task_desc);

 private:
  uint64_t ApplySchedulingDeltas(const vector<SchedulingDelta*>& deltas);
  uint64_t AssignNode(
      vector< map< uint64_t, uint64_t > >* extracted_flow,
      uint64_t node);
  void ApplyDeltas();
  bool CheckNodeType(uint64_t node, FlowNodeType_NodeType type);
  map<uint64_t, uint64_t>* GetMappings(
      vector< map< uint64_t, uint64_t > >* extracted_flow,
      unordered_set<uint64_t> leaves,
      uint64_t sink);
  void NodeBindingToSchedulingDelta(const FlowGraphNode& src,
                                    const FlowGraphNode& dst,
                                    SchedulingDelta* delta);
  void PrintGraph(vector< map<uint64_t, uint64_t> > adj_map);
  vector< map< uint64_t, uint64_t> >* ReadFlowGraph(
      int fd, uint64_t num_vertices);
  void RegisterLocalResource(ResourceID_t res_id);
  void RegisterRemoteResource(ResourceID_t res_id);
  void HandleNginxJob();
  uint64_t RunSchedulingIteration();


  HAProxyController haproxy_controller_;

  TaskDescriptor* ProducingTaskForDataObjectID(DataObjectID_t id);
  // Cached sets of runnable and blocked tasks; these are updated on each
  // execution of LazyGraphReduction. Note that this set includes tasks from all
  // jobs.
  set<TaskID_t> runnable_tasks_;
  set<TaskDescriptor*> blocked_tasks_;
  // Initialized to hold the URI of the (currently unique) coordinator this
  // scheduler is associated with. This is passed down to the executor and to
  // tasks so that they can find the coordinator at runtime.
  const string coordinator_uri_;
  // We also record the resource ID of the owning coordinator.
  ResourceID_t coordinator_res_id_;
  map<ResourceID_t, ExecutorInterface*> executors_;
  map<TaskID_t, ResourceID_t> task_bindings_;
  // Pointer to the coordinator's topology manager
  shared_ptr<TopologyManager> topology_manager_;
  // Pointer to messaging adapter to use for communication with remote
  // resources.
  MessagingAdapterInterface<BaseMessage>* m_adapter_ptr_;
  // Flag (effectively a lock) indicating if the scheduler is currently
  // in the process of making scheduling decisions.
  bool scheduling_;
  // Local storage of the current flow graph
  FlowGraph *flow_graph_;
  // Flow scheduler parameters (passed in as protobuf to constructor)
  SchedulingParameters parameters_;
  // DIMACS exporter for interfacing to the solver
  DIMACSExporter exporter_;
  // Debug sequence number (for solver input/output files written to /tmp)
  uint64_t debug_seq_num_;
};

}  // namespace scheduler
}  // namespace firmament

#endif  // FIRMAMENT_ENGINE_ENERGY_SCHEDULER_H
