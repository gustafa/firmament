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

  // Load statistics to use when building graph costs.
  SetInitialStats();
 // ResourceStatsMap *nginx_stats = new ResourceStatsMap();
 //  SetInitialNginxStats(nginx_stats);
 //  application_host_stats_["nginx"] = nginx_stats;
}

Cost_t EnergyCostModel::TaskToUnscheduledAggCost(TaskID_t task_id, FlowSchedulingPriorityType priority) {
  // Not to be used in this cost model.
  CHECK(false);
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
  BatchAppStatistics *app_stats = dynamic_cast<BatchAppStatistics *>(FindPtrOrNull(*application_stats_, application));
  bool new_application = false;
  if (!app_stats) {
    VLOG(2) << "Did not find application " << application << " in the database. "
            << "Creating new.";
    app_stats = new BatchAppStatistics();
    // TODO setup the new, unseen program with defaults.
    (*application_stats_)[application] = app_stats;
    new_application = true;
  }


  CHECK(td->has_input_size());

  //ResourceID_t *bound_resource = FindOrNull(*task_bindings_, task_id);

  string task_host = "";
  // if (bound_resource) {
  //   string bound_host = (*resource_to_host_)[*bound_resource];
  // }
  // _Should_ exist.

  // Get the task_perf stats if it exists.
  const deque<TaskPerfStatisticsSample>* task_perf = knowledge_base_->GetStatsForTask(task_id);


  // Runtime, machine idx
  vector<pair<double, uint64_t>> runtimes;
  vector<double> powers;

  double total_schedulable_power = 0;

  // Set all as poor scheduling choices for now and then we update them later :).
  machine_task_costs.insert(machine_task_costs.begin(), machine_ids.size(), POOR_SCHEDULING_CHOICE);

  vector<uint64_t> possible_machine_idxs;

  for (uint64_t i = 0; i < machine_ids.size(); ++i) {
    ResourceID_t machine_id = machine_ids[i];
      // TODO handle case when we have started a task!
    double remaining = 1;
    string host = (*resource_to_host_)[machine_id];
    VLOG(2) << "Estimating cost of " << application  << " on "  << host;

    double runtime = remaining * app_stats->GetRuntime(host, td->input_size());

    runtimes.push_back(make_pair(runtime, i));
    double power = app_stats->GetPower(host);
    powers.push_back(power);

    if (td->has_absolute_deadline() &&
      (td->absolute_deadline() - GetCurrentTimestamp()) <= runtime) {
      // We are not expected to make the deadline, let the scheduler know.
      total_schedulable_power += power;
      possible_machine_idxs.push_back(i);
    }
    // Set all to poor scheduling choice initially, we'll change this later.
  }

  double fastest_runtime = min_element(runtimes.begin(), runtimes.end())->first;
  if (possible_machine_idxs.size()) {
    double min_cost = POOR_SCHEDULING_CHOICE;
    for (auto machine_idx : possible_machine_idxs) {
      double cost = powers[machine_idx] * runtimes[machine_idx].first / fastest_runtime;
      machine_task_costs[machine_idx] = cost;
      if (cost < min_cost) {
        min_cost = cost;
      }
    }
    if (possible_machine_idxs.size() > 2) {
      // Unscheduled 15 % more expensive than the cheapest available option
      return uint64_t(min_cost * 1.15);
    } else {
      // Less than two machines can still run this task on time!
      return uint64_t(min_cost * 20);
    }

  } else {
    // Shoot! We missed the deadline (no possible machines to schedule on) now lets wrap this thing up as quickly as possible.
    sort(runtimes.begin(), runtimes.end());
    // Consider a maximum of two resources.
    uint64_t num_considered = machine_ids.size() < 2 ? machine_ids.size() : 2;
    // Still use their energies but make it crazy expensive to not schedule this! :)
    for (uint64_t i = 0; i < num_considered; ++i) {
      uint64_t machine_idx = runtimes[i].second;
      machine_task_costs[machine_idx] = powers[machine_idx] * runtimes[machine_idx].first / fastest_runtime;
    }
    // Do not unschedule por favor ;)
    return POOR_SCHEDULING_CHOICE;
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
  BatchAppStatistics *wc_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> uriel_wcmapred_runtimes({make_pair(1, 40.120000), make_pair(2, 80.660000), make_pair(3, 120.900000), make_pair(4, 162.200000), make_pair(6, 244.670000), make_pair(7, 283.780000), make_pair(10, 399.990000), make_pair(12, 478.530000)});
  vector<pair<uint64_t, double>> pandaboard_wcmapred_runtimes({make_pair(1, 278.820000), make_pair(2, 551.580000), make_pair(3, 828.310000), make_pair(4, 1111.680000), make_pair(6, 1656.250000)});
  vector<pair<uint64_t, double>> michael_wcmapred_runtimes({make_pair(1, 41.560000), make_pair(2, 84.740000), make_pair(3, 125.560000), make_pair(4, 163.600000), make_pair(6, 248.270000), make_pair(7, 290.850000), make_pair(10, 401.670000), make_pair(12, 479.280000)});
  vector<pair<uint64_t, double>> titanic_wcmapred_runtimes({make_pair(1, 165.680000), make_pair(2, 376.670000), make_pair(3, 573.770000), make_pair(4, 770.440000), make_pair(6, 1133.010000), make_pair(7, 1313.130000), make_pair(10, 1816.540000), make_pair(12, 2149.750000)});

  wc_stats->SetRuntimes("michael", michael_wcmapred_runtimes);
  wc_stats->SetRuntimes("uriel", uriel_wcmapred_runtimes);
  wc_stats->SetRuntimes("pandaboard", pandaboard_wcmapred_runtimes);
  wc_stats->SetRuntimes("titanic", titanic_wcmapred_runtimes);

  wc_stats->SetPower("uriel", 50.9101429584);
  wc_stats->SetPower("pandaboard", 1.171791979);
  wc_stats->SetPower("michael", 26.4011700789);
  wc_stats->SetPower("titanic", 92.7074207076);

  // Test wc_stats
  wc_stats->SetRuntimes("gustafa", uriel_wcmapred_runtimes);
  wc_stats->SetPower("gustafa", 10);


  (*application_stats_)[TaskDescriptor::MAPREDUCE_WC] = wc_stats;

  BatchAppStatistics *join_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> uriel_joinmapred_runtimes({make_pair(1, 46.730000), make_pair(2, 102.060000), make_pair(3, 159.680000), make_pair(4, 227.760000), make_pair(6, 382.210000), make_pair(7, 476.060000), make_pair(10, 782.670000), make_pair(12, 794.940000)});
  vector<pair<uint64_t, double>> pandaboard_joinmapred_runtimes({make_pair(1, 376.620000), make_pair(2, 866.160000), make_pair(3, 1483.490000), make_pair(4, 2229.270000), make_pair(7, 1611.000000), make_pair(10, 2173.000000), make_pair(12, 2166.000000)});
  vector<pair<uint64_t, double>> michael_joinmapred_runtimes({make_pair(1, 48.940000), make_pair(2, 99.850000), make_pair(3, 159.640000), make_pair(4, 219.070000), make_pair(6, 365.990000), make_pair(7, 432.600000), make_pair(10, 741.300000), make_pair(12, 751.060000)});
  vector<pair<uint64_t, double>> titanic_joinmapred_runtimes({make_pair(3, 953.760000), make_pair(7, 3412.840000), make_pair(10, 2229.000000), make_pair(12, 2177.000000)});
  join_stats->SetRuntimes("michael", michael_joinmapred_runtimes);
  join_stats->SetRuntimes("uriel", uriel_joinmapred_runtimes);
  join_stats->SetRuntimes("pandaboard", pandaboard_joinmapred_runtimes);
  join_stats->SetRuntimes("titanic", titanic_joinmapred_runtimes);
  join_stats->SetPower("uriel", 51.4161912923);
  join_stats->SetPower("pandaboard", 0.954096364266);
  join_stats->SetPower("michael", 26.2885511978);
  join_stats->SetPower("titanic", 8.56867123191);


  join_stats->SetRuntimes("gustafa", michael_joinmapred_runtimes);
  join_stats->SetPower("gustafa", 10);

  (*application_stats_)[TaskDescriptor::MAPREDUCE_WC] = join_stats;

  BatchAppStatistics *filetransfer_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> gustafa_filetransfer_runtimes({make_pair(1, 60), make_pair(10, 600)});
  filetransfer_stats->SetPower("gustafa", 20);
  filetransfer_stats->SetRuntimes("gustafa", gustafa_filetransfer_runtimes);

  (*application_stats_)[TaskDescriptor::FILETRANSFER] = filetransfer_stats;

}

TaskType EnergyCostModel::GetTaskType(TaskID_t task_id) {
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  return (*td)->task_type();
}

}  // namespace firmament
