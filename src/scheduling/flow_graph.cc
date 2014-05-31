// The Firmament project
// Copyright (c) 2013 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Representation of a Quincy-style scheduling flow graph.

#include <string>
#include <queue>

#include <cstdio>

#include <boost/bind.hpp>

#include "base/common.h"
#include "base/types.h"
#include "misc/equivclasses.h"
#include "misc/map-util.h"
#include "misc/pb_utils.h"
#include "misc/string_utils.h"
#include "misc/utils.h"
#include "scheduling/flow_graph.h"
#include "scheduling/flow_scheduling_cost_model_interface.h"

DEFINE_bool(preemption, false, "Enable preemption and migration of tasks");

namespace firmament {

using machine::topology::TopologyManager;

FlowGraph::FlowGraph(FlowSchedulingCostModelInterface *cost_model)
    : cost_model_(cost_model),
      current_id_(1) {
  // Add sink and cluster aggregator node
  AddSpecialNodes();
}

FlowGraph::~FlowGraph() {
  delete cost_model_;
  // XXX(malte): N.B. this leaks memory as we haven't destroyed all of the
  // nodes and arcs in the flow graph (which are allocated on the heap)
}

void FlowGraph::AddArcsForTask(FlowGraphNode* task_node,
                               FlowGraphNode* unsched_agg_node) {
  // We always have an edge to the cluster aggregator node
  FlowGraphArc* cluster_agg_arc = AddArcInternal(task_node, cluster_agg_node_);
  // Assign cost to the (task -> cluster agg) edge from cost model
  cluster_agg_arc->cost_ =
      cost_model_->TaskToClusterAggCost(task_node->task_id_);
      // TODO(gustafa): Verify, do we want this?
  cluster_agg_arc->cap_upper_bound_ = 1;
  uint64_t schedulable_on = 0;

  // Adding arcs to the individual machines in the topology.
  for (auto &keyval : cluster_agg_node_->outgoing_arc_map_) {
    uint64_t node_id = keyval.first;

    FlowGraphNode *node = Node(node_id);

    Cost_t task_cost = cost_model_->TaskToResourceNodeCost(task_node->task_id_, node->resource_id_);

    // Create an arc and set the cost if the cost model found find it appropriate.
    if (task_cost != cost_model_->POOR_SCHEDULING_CHOICE) {
      FlowGraphArc *task_node_arc = AddArcInternal(task_node, node);
      task_node_arc->cost_ = task_cost;
      ++schedulable_on;
    }
  }

  // We also always have an edge to our job's unscheduled node
  FlowGraphArc* unsched_arc = AddArcInternal(task_node, unsched_agg_node);
  // Add this task's potential flow to the per-job unscheduled
  // aggregator's outgoing edge
  AdjustUnscheduledAggToSinkCapacity(task_node->job_id_, 1);

  Cost_t unsched_arc_cost;
  // TODO Make a switch on priority levels to determine what is low, medium and high respectively.
  unsched_arc->cap_upper_bound_ = 1;
  if (!schedulable_on) {

    // If the cost model did not find any appropriate candidate schedule the task
    // this typically means we want to schedule it ASAP as it is already missing
    // its expected performance.
    // We add an arc to the cluster aggregator and request high priority, i.e. high cost, for
    // not scheduling it

    FlowGraphArc *cluster_agg_arc = AddArcInternal(task_node, cluster_agg_node_);
    cluster_agg_arc->cost_ = cost_model_->TaskToClusterAggCost(task_node->task_id_);

    unsched_arc_cost = cost_model_->TaskToUnscheduledAggCost(task_node->task_id_, PRIORITY_LOW);

  } else {
    // By default we use a low priority if we are able to schedule the event.
    unsched_arc_cost = cost_model_->TaskToUnscheduledAggCost(task_node->task_id_, PRIORITY_LOW);
  }

  unsched_arc->cost_ = unsched_arc_cost;

}

FlowGraphArc* FlowGraph::AddArcInternal(uint64_t src, uint64_t dst) {
  FlowGraphArc* arc = new FlowGraphArc(src, dst);
  arc_set_.insert(arc);
  FlowGraphNode** src_node = FindOrNull(node_map_, src);
  CHECK_NOTNULL(src_node);
  (*src_node)->AddArc(arc);
  return arc;
}

FlowGraphArc* FlowGraph::AddArcInternal(FlowGraphNode* src,
                                        FlowGraphNode* dst) {
  FlowGraphArc* arc = new FlowGraphArc(src->id_, dst->id_);
  arc_set_.insert(arc);
  src->AddArc(arc);
  return arc;
}

FlowGraphNode* FlowGraph::AddEquivClassAggregator(
    TaskEquivClass_t equivclass) {
  uint64_t* equiv_class_node_id = FindOrNull(equiv_class_to_nodeid_map_,
                                             equivclass);
  if (!equiv_class_node_id) {
    // Need to add the equiv class aggregator first
    FlowGraphNode* ec_node = AddNodeInternal(next_id());
    equiv_class_node_id = &(ec_node->id_);
    InsertIfNotPresent(&equiv_class_to_nodeid_map_,
                       equivclass, *equiv_class_node_id);
    string comment;
    spf(&comment, "EC_AGG_%ju", equivclass);
    ec_node->comment_ = comment;
  }
#if 0
  // XXX(malte): HACK!
  if (equivclass == 9726732246984505783ULL) {
    // matmult
    VLOG(1) << "Adding EQUIV CLASS PREFERENCE EDGES for MATMULT!";
    string res[] = {"8fc55627-896e-4006-a716-e1c55507b384",
                    "a8169544-2709-4c80-82ca-f19879391b36"};
    //string res[] = {"8fc55627-896e-4006-a716-e1c55507b384",
    //                "377a2a8b-0ad7-4f4c-960a-884a6e00a06a"};
    for (uint64_t i = 0; i < 2; ++i) {
      uint64_t* res_node = FindOrNull(resource_to_nodeid_map_,
                                      ResourceIDFromString(res[i]));
      CHECK_NOTNULL(res_node);
      AddArcInternal(*equiv_class_node_id, *res_node);
    }
  } else if (equivclass == 1717579855873226448ULL) {
    VLOG(1) << "Adding EQUIV CLASS PREFERENCE EDGES for PIAPP!";
    // pi_app
    string res[] = {"377a2a8b-0ad7-4f4c-960a-884a6e00a06a",
                    "a3c93f23-798d-4b84-8683-c3ccacc38702"};
    //string res[] = {"a8169544-2709-4c80-82ca-f19879391b36",
    //                "a3c93f23-798d-4b84-8683-c3ccacc38702"};
    for (uint64_t i = 0; i < 2; ++i) {
      uint64_t* res_node = FindOrNull(resource_to_nodeid_map_,
                                      ResourceIDFromString(res[i]));
      CHECK_NOTNULL(res_node);
      AddArcInternal(*equiv_class_node_id, *res_node);
    }
  } else {
    // unknown, just rought through cluster agg
    VLOG(1) << "Adding NO EQUIV CLASS PREFERENCE EDGES as task UNKOWN!";
    AddArcInternal(*equiv_class_node_id, cluster_agg_node_->id_);
  }
#endif
  return Node(*equiv_class_node_id);
}

void FlowGraph::AddJobNodes(JobDescriptor* jd) {
  // First add an unscheduled aggregator node for this job
  // if none exists alread
  FlowGraphArc* unsched_agg_to_sink_arc;
  FlowGraphNode* unsched_agg_node;
  uint64_t* unsched_agg_node_id = FindOrNull(job_to_nodeid_map_,
                                             JobIDFromString(jd->uuid()));
  if (!unsched_agg_node_id) {
    unsched_agg_node = AddNodeInternal(next_id());
    unsched_agg_node->type_.set_type(FlowNodeType::JOB_AGGREGATOR);
    string comment;
    spf(&comment, "UNSCHED_AGG_for_%s", jd->uuid().c_str());
    unsched_agg_node->comment_ = comment;
    // ... and connect it directly to the sink
    unsched_agg_to_sink_arc = AddArcInternal(unsched_agg_node, sink_node_);
    unsched_agg_to_sink_arc->cap_upper_bound_ = 0;
    unsched_agg_to_sink_arc->cost_ =
        cost_model_->UnscheduledAggToSinkCost(JobIDFromString(jd->uuid()));
    // Record this for the future in the job <-> node ID lookup table
    CHECK(InsertIfNotPresent(&job_to_nodeid_map_, JobIDFromString(jd->uuid()),
                             unsched_agg_node->id_));
  } else {
    FlowGraphNode** unsched_agg_node_ptr = FindOrNull(node_map_,
                                                      *unsched_agg_node_id);
    unsched_agg_node = *unsched_agg_node_ptr;
    FlowGraphArc** lookup_ptr = FindOrNull(unsched_agg_node->outgoing_arc_map_,
                                           sink_node_->id_);
    CHECK_NOTNULL(lookup_ptr);
    unsched_agg_to_sink_arc = *lookup_ptr;
  }
  // TODO(gustafa): Maybe clear this and just fill it up on every iteration instead of this first time.
  unsched_agg_nodes_.insert(unsched_agg_node->id_);

  // Now add the job's task nodes
  // TODO(malte): This is a simple BFS lashup; maybe we can do better?
  queue<TaskDescriptor*> q;
  q.push(jd->mutable_root_task());
  while (!q.empty()) {
    TaskDescriptor* cur = q.front();
    q.pop();
    // Check if this node has already been added
    uint64_t* tn_ptr = FindOrNull(task_to_nodeid_map_, cur->uid());
    FlowGraphNode* task_node = tn_ptr ? Node(*tn_ptr) : NULL;
    if (cur->state() == TaskDescriptor::RUNNABLE && !task_node) {
      task_node = AddNodeInternal(next_id());
      task_node->type_.set_type(FlowNodeType::UNSCHEDULED_TASK);
      // Add the current task's node
      task_node->supply_ = 1;
      task_node->task_id_ = cur->uid();  // set task ID in node
      task_node->job_id_ = JobIDFromString(jd->uuid());
      sink_node_->demand_++;
      task_nodes_.insert(task_node->id_);
      // Insert a record for the node representing this task's ID
      InsertIfNotPresent(&task_to_nodeid_map_, cur->uid(), task_node->id_);
      // Log info
      VLOG(2) << "Adding edges for task " << cur->uid() << "'s node ("
              << task_node->id_ << "); task state is " << cur->state();
      // Arcs for this node
      AddArcsForTask(task_node, unsched_agg_node);
      // XXX(malte): hack to add equiv class aggregator nodes
      VLOG(2) << "Equiv class for task " << cur->uid() << " is "
              << GenerateTaskEquivClass(*cur);
      FlowGraphNode* ec_node =
          AddEquivClassAggregator(GenerateTaskEquivClass(*cur));
      FlowGraphArc* ec_arc = AddArcInternal(task_node->id_,
                                            ec_node->id_);
      ec_arc->cost_ = 0;
    } else if (cur->state() == TaskDescriptor::RUNNING ||
             cur->state() == TaskDescriptor::ASSIGNED) {
      // The task is already running, so it must have a node already
      //task_node->type_.set_type(FlowNodeType::SCHEDULED_TASK);
    } else if (task_node) {
      VLOG(2) << "Ignoring task " << cur->uid()
              << ", as its node already exists.";
    } else {
      VLOG(2) << "Ignoring task " << cur->uid() << " [" << hex << cur
              << "], which is in state "
              << ENUM_TO_STRING(TaskDescriptor::TaskState, cur->state());
    }
    // Enqueue any existing children of this task
    for (RepeatedPtrField<TaskDescriptor>::iterator c_iter =
         cur->mutable_spawned()->begin();
         c_iter != cur->mutable_spawned()->end();
         ++c_iter) {
      // We do actually need to push tasks even if they are already completed,
      // failed or running, since they may have children eligible for
      // scheduling.
      q.push(&(*c_iter));
    }
  }
  // Set the supply on the unscheduled node to the difference between the
  // maximum number of running tasks for this job and the number of tasks
  // (F_j - N_j in Quincy terms).
  // TODO(malte): Stub -- this currently allows an unlimited number of tasks per
  // job to be scheduled.
  unsched_agg_node->supply_ = 0;
}

FlowGraphNode* FlowGraph::AddNodeInternal(uint64_t id) {
  FlowGraphNode* node = new FlowGraphNode(id);
  CHECK(InsertIfNotPresent(&node_map_, id, node));
  return node;
}

void FlowGraph::AddSpecialNodes() {
  // N.B.: we do NOT create a cluster aggregator node X here, since
  // the root of the resource topology is automatically chosen as the
  // cluster aggregator.
  // Sink node
  sink_node_ = AddNodeInternal(next_id());
  sink_node_->type_.set_type(FlowNodeType::SINK);
  sink_node_->comment_ = "SINK";
}

void FlowGraph::AddResourceTopology(
    const ResourceTopologyNodeDescriptor& resource_tree) {
  TraverseResourceProtobufTreeReturnRTND(
      resource_tree,
      boost::bind(&FlowGraph::AddResourceNode, this, _1));
}

void FlowGraph::AddResourceNode(const ResourceTopologyNodeDescriptor& rtnd) {
  FlowGraphNode* new_node;
  // Add the node if it does not already exist
  if (!NodeForResourceID(ResourceIDFromString(rtnd.resource_desc().uuid()))) {
    uint64_t id = next_id();
    VLOG(2) << "Adding node " << id << " for resource "
            << rtnd.resource_desc().uuid();
    new_node = AddNodeInternal(id);
    InsertIfNotPresent(&resource_to_nodeid_map_,
                       ResourceIDFromString(rtnd.resource_desc().uuid()),
                       new_node->id_);
    new_node->resource_id_ = ResourceIDFromString(rtnd.resource_desc().uuid());
    // Record the parent if we have one
    if (rtnd.has_parent_id()) {
      // Add arc from parent to us if it doesn't already exist
      FlowGraphNode* parent_node =
        NodeForResourceID(ResourceIDFromString(rtnd.parent_id()));
      FlowGraphArc** arc = FindOrNull(parent_node->outgoing_arc_map_, id);
      if (!arc) {
        VLOG(2) << "Adding missing arc from parent "
                << parent_node->resource_id_
                << "(" << parent_node->id_ << ") to "
                << rtnd.resource_desc().uuid() << "("  << id << ").";
        AddArcInternal(parent_node->id_, id);
      }
      InsertIfNotPresent(&resource_to_parent_map_,
                         new_node->resource_id_,
                         ResourceIDFromString(rtnd.parent_id()));
    }
  } else {
    new_node = NodeForResourceID(
        ResourceIDFromString(rtnd.resource_desc().uuid()));
  }
  // Consider different cases: root node, internal node and leaf node
  // N.B.: the root node is processed BOTH by the root case (1) and the
  // branch or leaf case (2 or 3).
  if (!rtnd.has_parent_id()) {
    // 1) Root node
    ConfigureResourceRootNode(rtnd, new_node);
  }
  if (rtnd.children_size() > 0) {
    // 2) Node inside the tree with non-zero children (i.e. no leaf node)
    ConfigureResourceBranchNode(rtnd, new_node);
  } else if (rtnd.has_parent_id()) {
    // 3) Leaves of the resource topology; add an arc to the sink node
    ConfigureResourceLeafNode(rtnd, new_node);
  } else {
    LOG(WARNING) << "Orphan node in resource toplogy: it has neither children "
                 << "nor a parent! (resource id: " << rtnd.resource_desc().uuid();
  }
}

void FlowGraph::AdjustUnscheduledAggToSinkCapacity(JobID_t job, int64_t delta) {
  uint64_t* unsched_agg_node_id = FindOrNull(job_to_nodeid_map_, job);
  CHECK_NOTNULL(unsched_agg_node_id);
  FlowGraphArc** lookup_ptr =
      FindOrNull(Node(*unsched_agg_node_id)->outgoing_arc_map_,
                 sink_node_->id_);
  CHECK_NOTNULL(lookup_ptr);
  FlowGraphArc* unsched_agg_to_sink_arc = *lookup_ptr;
  unsched_agg_to_sink_arc->cap_upper_bound_ += delta;
}

void FlowGraph::ConfigureResourceRootNode(
    const ResourceTopologyNodeDescriptor& rtnd, FlowGraphNode* new_node) {
  // 1) Root node
  // N.B. a root node without parent is always automatically taken as the
  // cluster aggregator.
  new_node->type_.set_type(FlowNodeType::GLOBAL_AGGREGATOR);
  new_node->comment_ = "CLUSTER_AGG";
  // Reset cluster aggregator to this node
  cluster_agg_node_ = new_node;
}

void FlowGraph::ConfigureResourceBranchNode(
    const ResourceTopologyNodeDescriptor& rtnd, FlowGraphNode* new_node) {
  // Add internal arc from parent
  if (rtnd.has_parent_id()) {
    FlowGraphNode* parent_node = NodeForResourceID(
        ResourceIDFromString(rtnd.parent_id()));
    CHECK(parent_node != NULL) << "Could not find parent node with ID "
                               << rtnd.parent_id();
    // Find the arc from parent node (which should have been added before)
    FlowGraphArc** arc_ptr = FindOrNull(parent_node->outgoing_arc_map_,
                                        new_node->id_);
    CHECK_NOTNULL(arc_ptr);
    FlowGraphArc* arc = *arc_ptr;
    // Set initial capacity to 0; this will be updated as leaves are added
    // below this node!
    arc->cap_upper_bound_ = 0;
    arc->cost_ =
        cost_model_->ResourceNodeToResourceNodeCost(
            parent_node->resource_id_, new_node->resource_id_);
  } else if (new_node->type_.type() != FlowNodeType::GLOBAL_AGGREGATOR) {
    // Having no parent is only okay if we're the root node
    LOG(FATAL) << "Found child without parent_id set! This will lead to an "
               << "inconsistent flow graph! child ID: "
               << rtnd.resource_desc().uuid();
  }
}

void FlowGraph::ConfigureResourceLeafNode(
    const ResourceTopologyNodeDescriptor& rtnd, FlowGraphNode* new_node) {
  VLOG(2) << "Considering node " << rtnd.resource_desc().uuid()
          << ", which has parent "
          << (rtnd.has_parent_id() ? rtnd.parent_id() : "NONE");
  VLOG(2) << "Adding arc from leaf resource " << rtnd.resource_desc().uuid()
          << " to sink node.";
  if (rtnd.resource_desc().type() != ResourceDescriptor::RESOURCE_PU)
    LOG(FATAL) << "Leaf resource " << rtnd.resource_desc().uuid()
               << " is not a PU! This may yield an unschedulable flow!";
  FlowGraphNode* cur_node = NodeForResourceID(
      ResourceIDFromString(rtnd.resource_desc().uuid()));
  CHECK(cur_node != NULL) << "Could not find leaf node with ID "
                          << rtnd.resource_desc().uuid();
  cur_node->type_.set_type(FlowNodeType::PU);
  FlowGraphArc* arc = AddArcInternal(cur_node->id_, sink_node_->id_);
  arc->cap_upper_bound_ = 1;  // TODO(malte): change this if support time-sharing
  arc->cost_ =
      cost_model_->LeafResourceNodeToSinkCost(cur_node->resource_id_);
  leaf_nodes_.insert(cur_node->id_);
  // Add flow capacity to parent nodes until we hit the root node
  FlowGraphNode* parent = cur_node;
  ResourceID_t* parent_id;
  while ((parent_id = FindOrNull(resource_to_parent_map_,
                                 parent->resource_id_)) != NULL) {
    uint64_t cur_id = parent->id_;
    parent = NodeForResourceID(*parent_id);
    FlowGraphArc** arc = FindOrNull(parent->outgoing_arc_map_, cur_id);
    CHECK_NOTNULL(arc);
    CHECK_NOTNULL(*arc);
    VLOG(2) << "Adding capacity on edge from " << *parent_id << " ("
            << parent->id_ << ") to " << cur_id << " ("
            << (*arc)->cap_upper_bound_ << " -> "
            << (*arc)->cap_upper_bound_ + 1 << ")";
    (*arc)->cap_upper_bound_ += 1;
  }
}

void FlowGraph::DeleteArc(FlowGraphArc* arc) {
  // First remove various meta-data relating to this arc
  arc_set_.erase(arc);
  // Then delete the arc itself
  delete arc;
}

void FlowGraph::DeleteTaskNode(FlowGraphNode* node) {
  // First remove all outgoing arcs
  for (unordered_map<uint64_t, FlowGraphArc*>::iterator it =
      node->outgoing_arc_map_.begin();
      it != node->outgoing_arc_map_.end();
      ++it) {
    DeleteArc(it->second);
  }
  node->outgoing_arc_map_.clear();
  // Decrease the sink's demand and set this node's supply to zero
  node->supply_ = 0;
  sink_node_->demand_--;
  // Find the unscheduled node for this job and decrement its outgoing capacity
  // TODO(malte): this is only relevant if we support preemption; otherwise the
  // capcacity will already have been deducted (as part of PinTaskToNode,
  // currently).
  // Then remove node meta-data
  VLOG(2) << "Deleting task node with id " << node->id_ << ", task id " << node->task_id_;
  node_map_.erase(node->id_);
  task_nodes_.erase(node->task_id_);
  // Then remove the node itself
  delete node;
}

FlowGraphNode* FlowGraph::NodeForResourceID(const ResourceID_t& res_id) {
  uint64_t* id = FindOrNull(resource_to_nodeid_map_, res_id);
  // Returns NULL if resource unknown
  if (!id)
    return NULL;
  VLOG(2) << "Resource " << res_id << " is represented by node " << *id;
  FlowGraphNode** node_ptr = FindOrNull(node_map_, *id);
  return (node_ptr ? *node_ptr : NULL);
}

FlowGraphNode* FlowGraph::NodeForTaskID(TaskID_t task_id) {
  uint64_t* id = FindOrNull(task_to_nodeid_map_, task_id);
  // Returns NULL if task unknown
  if (!id)
    return NULL;
  VLOG(2) << "Task " << task_id << " is represented by node " << *id;
  FlowGraphNode** node_ptr = FindOrNull(node_map_, *id);

  return (node_ptr ? *node_ptr : NULL);
}

void FlowGraph::PinTaskToNode(FlowGraphNode* task_node,
                              FlowGraphNode* res_node) {
  // Remove all arcs apart from the task -> resource mapping;
  // note that this effectively disables preemption!
  // ----
  // N.B.: we need to collect a set of pointers here rather than
  // deleting things inside the loop, as otherwise the iterator
  // gets confused
  set<TaskID_t> to_delete;
  for (unordered_map<TaskID_t, FlowGraphArc*>::iterator it =
       task_node->outgoing_arc_map_.begin();
       it != task_node->outgoing_arc_map_.end();
       ++it) {
    VLOG(2) << "Deleting arc from " << it->second->src_ << " to "
            << it->second->dst_;
    DeleteArc(it->second);
    to_delete.insert(it->first);
  }
  // N.B. This is a little dodgy, as it mutates the collection inside the
  // loop. However, since nobody else is reading from it at the same time,
  // this should be fine.
  for (set<TaskID_t>::iterator it = to_delete.begin();
       it != to_delete.end();
       ++it) {
    task_node->outgoing_arc_map_.erase(*it);
  }
  // Remove this task's potential flow from the per-job unscheduled
  // aggregator's outgoing edge
  AdjustUnscheduledAggToSinkCapacity(task_node->job_id_, -1);
  // Re-add a single arc from the task to the resource node
  FlowGraphArc *arc = AddArcInternal(task_node, res_node);
  arc->cap_upper_bound_ = 1;
}

void FlowGraph::UpdateArcsForBoundTask(TaskID_t tid, ResourceID_t res_id) {
  FlowGraphNode* task_node = NodeForTaskID(tid);
  FlowGraphNode* assigned_res_node = NodeForResourceID(res_id);
  CHECK_NOTNULL(task_node);
  CHECK_NOTNULL(assigned_res_node);

  if (!FLAGS_preemption) {
    // After the task is bound, we now remove all of its edges into the flow
    // graph apart from the bound resource.
    // N.B.: This disables preemption and migration!
    VLOG(2) << "Disabling preemption for " << tid;
    // Disable preemption
    PinTaskToNode(task_node, assigned_res_node);
  }

}

void FlowGraph::UpdateResourceNode(const ResourceTopologyNodeDescriptor& rtnd) {
  ResourceID_t res_id = ResourceIDFromString(rtnd.resource_desc().uuid());
  // First of all, check if this node already exists in our resource topology
  uint64_t* found_node = FindOrNull(resource_to_nodeid_map_, res_id);
  VLOG(1) << "Considering resource " << res_id << ", which is "
          << (found_node ? *found_node : 0);
  if (found_node) {
    // Check if its parent is identical
    if (rtnd.has_parent_id()) {
      ResourceID_t* old_parent_id = FindOrNull(resource_to_parent_map_, res_id);
      ResourceID_t new_parent_id = ResourceIDFromString(rtnd.parent_id());
      if (!old_parent_id || *old_parent_id != new_parent_id) {
        // If not, we need to move it to the new parent
        InsertOrUpdate(&resource_to_parent_map_, res_id, new_parent_id);
        // Remove arc corresponding to the old parent/child relationship
        uint64_t* new_parent_node =
            FindOrNull(resource_to_nodeid_map_, new_parent_id);
        CHECK_NOTNULL(new_parent_node);
      }
      if (old_parent_id) {
        uint64_t* old_parent_node =
            FindOrNull(resource_to_nodeid_map_, *old_parent_id);
        CHECK_NOTNULL(old_parent_node);
        // TODO
      }
    }
    // Check if its children are identical
    // TODO
  } else {
    // It does not already exist, so add it.
    VLOG(1) << "Adding new resource " << res_id << " to flow graph.";
    // N.B.: We need to ensure we hook in at the right place here by setting the
    // parent ID appropriately if it is not already.
    AddResourceNode(rtnd);
  }
}

void FlowGraph::UpdateResourceTopology(
    const ResourceTopologyNodeDescriptor& resource_tree) {
  // N.B.: This only considers ADDITION of resources currently; if resources
  // are removed from the topology (e.g. due to a failure), they won't
  // disappear via this method.
  TraverseResourceProtobufTreeReturnRTND(
      resource_tree,
      boost::bind(&FlowGraph::UpdateResourceNode, this, _1));
  uint32_t new_num_leaves = 0;
  for (unordered_map<uint64_t, FlowGraphArc*>::const_iterator it =
       cluster_agg_node_->outgoing_arc_map_.begin();
       it != cluster_agg_node_->outgoing_arc_map_.end();
       ++it) {
    new_num_leaves += it->second->cap_upper_bound_;
  }
  VLOG(2) << "Updated resource topology in flow scheduler. New "
          << "number of schedulable leaves: "
          << (new_num_leaves);
}


}  // namespace firmament
