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
                  shared_ptr<ResourceHostMap_t> resource_to_host,  map<TaskID_t, ResourceID_t> *task_bindings)
  : resource_map_(resource_map),
  job_map_(job_map),
  task_map_(task_map),
  knowledge_base_(knowledge_base),
  resource_to_host_(resource_to_host),
  task_bindings_(task_bindings_) {
  application_stats_ = knowledge_base_->AppStats();
  CHECK_NOTNULL(task_bindings_);
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

      //return app_stats->WorstEnergy(); // TODO multiplier? ..TODO FIX
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
  // TaskDescriptor::TaskType application = GetTaskType(task_id);
  // string host = (*resource_to_host_)[resource_id];
  // VLOG(2) << "Estimating cost of " << application  << " on "  << host;

  // ApplicationStatistics *app_stats = FindPtrOrNull(*application_stats_, application);

  // if (!app_stats) {
  //   VLOG(2) << "Did not find application " << application << " in the database. "
  //           << "Creating new.";
  //   app_stats = new ApplicationStatistics();
  //   // TODO setup the new, unseen program with defaults.
  //   (*application_stats_)[application] = app_stats;
  //   return 20ULL;

  // } else {
  //   // Get the stat for this resource
  //   // TODO check if this is the resource the task is currently on and
  //   // apply a discount!

  //   TaskDescriptor *td = FindPtrOrNull(*task_map_, task_id);
  //   // _Should_ exist.
  //   CHECK_NOTNULL(td);

  //   // TODO handle case when we have started a task!
  //   double completed = 0;


  //   if (td->has_absolute_deadline() &&
  //     app_stats->GetRuntime(host, completed) > (td->absolute_deadline() - GetCurrentTimestamp())) {
  //     // We are not expected to make the deadline, let the scheduler know.
  //     return POOR_SCHEDULING_CHOICE;
  //   } else {
  //     return app_stats->GetEnergy(host, 0);
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

Cost_t EnergyCostModel::BatchTaskToResourceNodeCosts(TaskID_t task_id, TaskDescriptor *td, const vector<ResourceID_t> &machine_ids,
                                                vector<Cost_t> &machine_task_costs) {

  TaskType application = td->task_type();
  ApplicationStatistics *app_stats = FindPtrOrNull(*application_stats_, application);
  bool new_application = false;
  if (!app_stats) {
    VLOG(2) << "Did not find application " << application << " in the database. "
            << "Creating new.";
    app_stats = new ApplicationStatistics();
    // TODO setup the new, unseen program with defaults.
    (*application_stats_)[application] = app_stats;
    new_application = true;
  }


  CHECK(td->has_input_size());

  ResourceID_t *bound_resource = FindOrNull(*task_bindings_, task_id);

  string task_host = "";
  if (bound_resource) {
    string bound_host = (*resource_to_host_)[*bound_resource];
  }
  // _Should_ exist.

  // Get the task_perf stats if it exists.
  const deque<TaskPerfStatisticsSample>* task_perf = knowledge_base_->GetStatsForTask(task_id);


  vector<pair<double, uint64_t>> runtimes;
  vector<double> powers;

  uint64_t schedulable_on = 0;


  double total_schedulable_power = 0;

  // for (uint64_t i = 0; i < machine_ids.size(); ++i) {
  //   string host = (*resource_to_host_)[machine_ids[i]];
  //   double completed = 0;
  //   if (host == task_host) {
  //     // We are currently running this task on this machine.
  //     // UNIMPLEMENTED
  //     // completed = ...  discount to be added
  //   }
  //   //double runtime =
  //   //runtimes.push_back
  // }


  for (uint64_t i = 0; i < machine_ids.size(); ++i) {
    ResourceID_t machine_id = machine_ids[i];
      // TODO handle case when we have started a task!
    double completed = 0;
    string host = (*resource_to_host_)[machine_id];
    VLOG(2) << "Estimating cost of " << application  << " on "  << host;

    double runtime = app_stats->GetRuntime(host, td->input_size());
    runtimes.push_back(make_pair(runtime, i));

    if (td->has_absolute_deadline() &&
      app_stats->GetRuntime(host, completed) > (td->absolute_deadline() - GetCurrentTimestamp())) {
      // We are not expected to make the deadline, let the scheduler know.
      machine_task_costs.push_back(POOR_SCHEDULING_CHOICE);
    } else {
      ++schedulable_on;
      double power = app_stats->GetPower(host);
      powers.push_back(power);
      total_schedulable_power += power;
      machine_task_costs.push_back(Cost_t(power));
    }
  }
  if (!schedulable_on) {
    // Shoot! We missed the deadline, now lets wrap this thing up as quickly as possible.
    sort(runtimes.begin(), runtimes.end());
    const uint64_t num_considered = 2;
    // Still use their energies but make it crazy expensive to not schedule this! :)
    for (uint64_t i = 0; i < num_considered; ++i) {
      uint64_t machine_idx = runtimes[i].second;
      machine_task_costs[machine_idx] = powers[machine_idx];
    }
    // Do not unschedule por favore ;)
    return 1000000000ULL;
  } else {
    if (schedulable_on > 2) {
       return Cost_t(total_schedulable_power / double(schedulable_on));
    } else {
      // Only two more hosts can still schedule this. Time to get on it!
      CHECK(powers.size() > 0);
      return Cost_t(*(max_element(powers.begin(), powers.end())) + 5);
    }

  }
}


Cost_t EnergyCostModel::TaskToResourceNodeCosts(TaskID_t task_id, const vector<ResourceID_t> &machine_ids,
                                                vector<Cost_t> &machine_task_costs) {
  TaskDescriptor *td = FindPtrOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  TaskDescriptor::TaskType application = td->task_type();

  if (application != TaskDescriptor::NGINX) {
    return BatchTaskToResourceNodeCosts(task_id, td, machine_ids, machine_task_costs);
  }

  return 0;

}


void EnergyCostModel::SetInitialStats() {

  ApplicationStatistics *wc_stats = new ApplicationStatistics();
  //(*application_stats_)[TaskDescriptor::MAPREDUCE_WC] = wc_stats;

  //wc_stats->SetEnergy("tcp:localhost:8088", 300);
  //wc_stats->SetRuntime("tcp:localhost:8088", 4000);

}

TaskType EnergyCostModel::GetTaskType(TaskID_t task_id) {
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  return (*td)->task_type();
}

}  // namespace firmament
