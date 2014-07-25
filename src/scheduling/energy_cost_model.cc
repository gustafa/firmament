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
  task_bindings_(task_bindings) {
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
  machine_task_costs.insert(machine_task_costs.begin(), machine_ids.size(), POOR_SCHEDULING_CHOICE - 10000);

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
      machine_task_costs[machine_idx] = cost * MULTIPLIER_;
      if (cost < min_cost) {
        min_cost = cost;
      }
    }
    if (possible_machine_idxs.size() > 2) {
      // Unscheduled 15 % more expensive than the cheapest available option
      return uint64_t(min_cost * 1.15) * MULTIPLIER_;
    } else {
      // Less than two machines can still run this task on time!
      return uint64_t(min_cost * 20)  * MULTIPLIER_;
    }

  } else {
    // Shoot! We missed the deadline (no possible machines to schedule on) now lets wrap this thing up as quickly as possible.
    sort(runtimes.begin(), runtimes.end());
    // Consider a maximum of two resources.
    uint64_t num_considered = machine_ids.size() < 2 ? machine_ids.size() : 2;
    // Still use their energies but make it crazy expensive to not schedule this! :)
    for (uint64_t i = 0; i < num_considered; ++i) {
      uint64_t machine_idx = runtimes[i].second;
      machine_task_costs[machine_idx] = (powers[machine_idx] * runtimes[machine_idx].first / fastest_runtime) * MULTIPLIER_;
    }
    // Do not unschedule por favor ;)
    return POOR_SCHEDULING_CHOICE - 10000;
  }
}


Cost_t EnergyCostModel::ServiceTaskToResourceNodeCosts(TaskID_t task_id, TaskDescriptor *td, const vector<ResourceID_t> &machine_ids,
                                                vector<Cost_t> &machine_task_costs) {
  TaskType application = td->task_type();
  ServiceAppStatistics *app_stats = dynamic_cast<ServiceAppStatistics *>(FindPtrOrNull(*application_stats_, application));
  bool new_application = false;
  if (!app_stats) {
    VLOG(2) << "Did not find application " << application << " in the database. "
            << "Creating new.";
    app_stats = new ServiceAppStatistics();
    // TODO setup the new, unseen program with defaults.
    (*application_stats_)[application] = app_stats;
    new_application = true;
  }


  CHECK(td->has_input_size());

  //ResourceID_t *bound_resource = FindOrNull(*task_bindings_, task_id);
  uint64_t input_size = td->input_size();
  string task_host = "";
  // if (bound_resource) {
  //   string bound_host = (*resource_to_host_)[*bound_resource];
  // }
  // _Should_ exist.

  // Get the task_perf stats if it exists.
  const deque<TaskPerfStatisticsSample>* task_perf = knowledge_base_->GetStatsForTask(task_id);

  // Runtime, machine idx
  vector<pair<uint64_t, uint64_t>> max_rpss;
  vector<double> powers;


  // Set all as poor scheduling choices for now and then we update them later :).
  machine_task_costs.insert(machine_task_costs.begin(), machine_ids.size(), POOR_SCHEDULING_CHOICE);

  vector<uint64_t> possible_machine_idxs;
  vector<string> hosts;
  for (uint64_t i = 0; i < machine_ids.size(); ++i) {
    ResourceID_t machine_id = machine_ids[i];
      // TODO handle case when we have started a task!
    string host = (*resource_to_host_)[machine_id];
    hosts.push_back(host);

    VLOG(2) << "Estimating cost of " << application  << " on "  << host;

    uint64_t max_rps = app_stats->MaxRPS(host);
    max_rpss.push_back(make_pair(max_rps, i));

    double power = app_stats->GetPower(host, input_size);
    powers.push_back(power);

    // TODO: verify this!
    //if (input_size > max_rps) {
    if (!knowledge_base_->NumRunningWebservers(host))
      possible_machine_idxs.push_back(i);
    //}
    // Set all to poor scheduling choice initially, we'll change this later.
  }

  if (possible_machine_idxs.size()) {
    for (auto machine_idx : possible_machine_idxs) {
      // TODO apply discount if running on task already... QQ
      machine_task_costs[machine_idx] = powers[machine_idx] * MULTIPLIER_;;
    }
  } else {
    // Shoot! We can't keep up with the current RPS (no possible machines to schedule on) now lets do our best
    sort(max_rpss.begin(), max_rpss.end());
    // Consider a maximum of two resources.
    uint64_t num_considered = machine_ids.size() < 2 ? machine_ids.size() : 2;
    // Still use their energies but make it crazy expensive to not schedule this! :)
    for (uint64_t i = 0; i < num_considered; ++i) {
      uint64_t machine_idx = max_rpss[i].second;
      // Refuse to run more than one per machine. TODO make this a bit more flexible.
      if (!knowledge_base_->NumRunningWebservers(hosts[machine_idx])) {
         machine_task_costs[machine_idx] = powers[machine_idx] * MULTIPLIER_;
      }
    }
  }

  // Not running a service task is always a bad ide
  return POOR_SCHEDULING_CHOICE - 100;


}

Cost_t EnergyCostModel::TaskToResourceNodeCosts(TaskID_t task_id, const vector<ResourceID_t> &machine_ids,
                                                vector<Cost_t> &machine_task_costs) {
  TaskDescriptor *td = FindPtrOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  TaskDescriptor::TaskType application = td->task_type();

  if (application == TaskDescriptor::NGINX) {
    return ServiceTaskToResourceNodeCosts(task_id, td, machine_ids, machine_task_costs);
  } else {
    return BatchTaskToResourceNodeCosts(task_id, td, machine_ids, machine_task_costs);
  }

  return 0;
}


void EnergyCostModel::SetInitialStats() {
  BatchAppStatistics *wc_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> uriel_wcmapred_runtimes({make_pair(1, 40.120000), make_pair(2, 80.660000), make_pair(3, 120.900000), make_pair(4, 162.200000), make_pair(6, 244.670000), make_pair(7, 283.780000), make_pair(10, 399.990000), make_pair(12, 478.530000)});
  vector<pair<uint64_t, double>> pandaboard_wcmapred_runtimes({make_pair(1, 278.820000), make_pair(2, 551.580000), make_pair(3, 828.310000), make_pair(4, 1111.680000), make_pair(6, 1656.250000), make_pair(12, 3336)});
  vector<pair<uint64_t, double>> michael_wcmapred_runtimes({make_pair(1, 41.560000), make_pair(2, 84.740000), make_pair(3, 125.560000), make_pair(4, 163.600000), make_pair(6, 248.270000), make_pair(7, 290.850000), make_pair(10, 401.670000), make_pair(12, 479.280000)});
  vector<pair<uint64_t, double>> titanic_wcmapred_runtimes({make_pair(1, 165.680000), make_pair(2, 376.670000), make_pair(3, 573.770000), make_pair(4, 770.440000), make_pair(6, 1133.010000), make_pair(7, 1313.130000), make_pair(10, 1816.540000), make_pair(12, 2149.750000)});
  vector<pair<uint64_t, double>> wandboard_wcmapred_runtimes({make_pair(1, 203.370000), make_pair(2, 405.880000), make_pair(7, 1409.480000), make_pair(10, 2001.150000), make_pair(12, 2410.130000)});


  wc_stats->SetRuntimes("michael", michael_wcmapred_runtimes);
  wc_stats->SetRuntimes("uriel", uriel_wcmapred_runtimes);
  wc_stats->SetRuntimes("pandaboard", pandaboard_wcmapred_runtimes);
  wc_stats->SetRuntimes("wandboard", wandboard_wcmapred_runtimes);

  wc_stats->SetRuntimes("titanic", titanic_wcmapred_runtimes);

  wc_stats->SetPower("uriel", 50.9101429584);
  wc_stats->SetPower("pandaboard", 1.171791979);
  wc_stats->SetPower("wandboard", 1.463973011);

  wc_stats->SetPower("michael", 26.4011700789);
  wc_stats->SetPower("titanic", 92.7074207076);

  // Test wc_stats
  wc_stats->SetRuntimes("gustafa", uriel_wcmapred_runtimes);
  wc_stats->SetPower("gustafa", 10);


  (*application_stats_)[TaskDescriptor::MAPREDUCE_WC] = wc_stats;

  BatchAppStatistics *join_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> uriel_joinmapred_runtimes({make_pair(0, 0), make_pair(1, 46.730000), make_pair(2, 102.060000), make_pair(3, 159.680000), make_pair(4, 227.760000), make_pair(6, 382.210000), make_pair(7, 476.060000), make_pair(10, 782.670000), make_pair(12, 794.940000)});
  vector<pair<uint64_t, double>> pandaboard_joinmapred_runtimes({make_pair(0, 0), make_pair(1, 376.620000), make_pair(2, 866.160000), make_pair(3, 1483.490000), make_pair(4, 2229.270000), make_pair(7, 1611.000000), make_pair(10, 2173.000000), make_pair(12, 2166.000000)});
  vector<pair<uint64_t, double>> michael_joinmapred_runtimes({make_pair(0, 0), make_pair(1, 48.940000), make_pair(2, 99.850000), make_pair(3, 159.640000), make_pair(4, 219.070000), make_pair(6, 365.990000), make_pair(7, 432.600000), make_pair(10, 741.300000), make_pair(12, 751.060000)});
  vector<pair<uint64_t, double>> titanic_joinmapred_runtimes({make_pair(0, 0), make_pair(1, 300), make_pair(3, 953.760000), make_pair(7, 3412.840000), make_pair(10, 2229.000000), make_pair(12, 2177.000000)});

                                                            // TODO REMOVE THIS ONE! (1,300) Titanic
  vector<pair<uint64_t, double>> wandboard_joinmapred_runtimes({make_pair(1, 279.090000), make_pair(2, 640.310000), make_pair(3, 1105.310000), make_pair(4, 1655.680000), make_pair(6, 3004.250000), make_pair(7, 3886.000000), make_pair(10, 6805.000000), make_pair(12, 6822.000000)});


  join_stats->SetRuntimes("michael", michael_joinmapred_runtimes);
  join_stats->SetRuntimes("uriel", uriel_joinmapred_runtimes);
  join_stats->SetRuntimes("pandaboard", pandaboard_joinmapred_runtimes);
  join_stats->SetRuntimes("wandboard", wandboard_joinmapred_runtimes);
  join_stats->SetRuntimes("titanic", titanic_joinmapred_runtimes);

  join_stats->SetPower("uriel", 51.4161912923);
  join_stats->SetPower("pandaboard", 0.954096364266);
  join_stats->SetPower("wandboard", 1.26559105625);
  join_stats->SetPower("michael", 26.2885511978);
  join_stats->SetPower("titanic", 8.56867123191);


  join_stats->SetRuntimes("gustafa", michael_joinmapred_runtimes);
  join_stats->SetPower("gustafa", 10);

  (*application_stats_)[TaskDescriptor::MAPREDUCE_JOIN] = join_stats;

  BatchAppStatistics *filetransfer_stats = new BatchAppStatistics();

  vector<pair<uint64_t, double>> uriel_filetransfer_runtimes({make_pair(0, 0), make_pair(10, 22.880000), make_pair(20, 45.210000), make_pair(50, 116.880000), make_pair(100, 229.980000), make_pair(150, 346.700000), make_pair(250, 577.120000), make_pair(400, 925.570000), make_pair(600, 1395.030000)});
  vector<pair<uint64_t, double>> pandaboard_filetransfer_runtimes({make_pair(0, 0), make_pair(10, 215.060000), make_pair(20, 435.680000), make_pair(50, 1099.320000), make_pair(100, 2300.210000), make_pair(150, 3395.120000), make_pair(250, 1972.000000), make_pair(400, 1695.000000), make_pair(600, 2658.000000)});
  vector<pair<uint64_t, double>> michael_filetransfer_runtimes({make_pair(0, 0), make_pair(10, 37.940000), make_pair(20, 73.080000), make_pair(50, 123.740000), make_pair(100, 266.990000), make_pair(150, 452.980000), make_pair(250, 696.060000), make_pair(400, 1160.330000), make_pair(600, 1720.590000)});
  vector<pair<uint64_t, double>> titanic_filetransfer_runtimes({make_pair(0, 0), make_pair(10, 44.440000), make_pair(20, 68.520000), make_pair(50, 230.590000), make_pair(100, 449.520000), make_pair(150, 736.880000), make_pair(250, 1180.450000), make_pair(400, 1901.510000), make_pair(600, 2878.910000)});
  vector<pair<uint64_t, double>> wandboard_filetransfer_runtimes({make_pair(20, 105.060000), make_pair(50, 255.990000), make_pair(100, 522.540000), make_pair(150, 783.250000), make_pair(250, 1307.340000), make_pair(400, 2100.130000), make_pair(600, 3157.830000)});

  filetransfer_stats->SetPower("uriel", 29.006090878);
  filetransfer_stats->SetPower("pandaboard", 1.39785491627);
  filetransfer_stats->SetPower("wandboard", 4.32086526915);
  filetransfer_stats->SetPower("michael", 18.5887076174);
  filetransfer_stats->SetPower("titanic", 13.7984403934);
  filetransfer_stats->SetPower("gustafa", 20);
  filetransfer_stats->SetRuntimes("gustafa", michael_filetransfer_runtimes);


  filetransfer_stats->SetRuntimes("michael", michael_filetransfer_runtimes);
  filetransfer_stats->SetRuntimes("uriel", uriel_filetransfer_runtimes);
  filetransfer_stats->SetRuntimes("titanic", titanic_filetransfer_runtimes);
  filetransfer_stats->SetRuntimes("pandaboard", pandaboard_filetransfer_runtimes);
  filetransfer_stats->SetRuntimes("wandboard", wandboard_filetransfer_runtimes);


  (*application_stats_)[TaskDescriptor::FILETRANSFER] = filetransfer_stats;


  ServiceAppStatistics *nginx_stats = new ServiceAppStatistics();

  vector<pair<uint64_t, double>> uriel_nginx_powers({make_pair(0, 0), make_pair(59, 9.492063), make_pair(148, 19.645347), make_pair(296, 30.315228), make_pair(592, 33.088226), make_pair(1184, 35.784197), make_pair(2368, 40.225777), make_pair(2960, 41.316929), make_pair(4440, 42.981031), make_pair(5920, 65.765314), make_pair(7400, 70.365278), make_pair(8880, 71.770542), make_pair(11840, 71.722372), make_pair(14800, 72.395053), make_pair(17760, 70.122766), make_pair(20720, 70.620190), make_pair(23680, 69.626921), make_pair(26640, 70.179305), make_pair(29600, 68.216350), make_pair(44400, 66.200352), make_pair(59200, 65.727352), make_pair(74000, 71.520575)});
  vector<pair<uint64_t, double>> pandaboard_nginx_powers({make_pair(0, 0),make_pair(59, 0.182516), make_pair(148, 0.182516), make_pair(296, 0.279908), make_pair(592, 0.432635), make_pair(1184, 0.621603), make_pair(2368, 0.612184), make_pair(2960, 0.458365), make_pair(4440, 0.629414), make_pair(5920, 0.579137), make_pair(7400, 0.623135), make_pair(8880, 0.447756), make_pair(11840, 0.461233), make_pair(14800, 0.375400), make_pair(17760, 0.564757), make_pair(20720, 0.867792), make_pair(23680, 0.688734), make_pair(26640, 0.648295), make_pair(29600, 0.653512), make_pair(44400, 0.816684), make_pair(59200, 0.808590), make_pair(74000, 1.054814), make_pair(88800, 1.036972), make_pair(118400, 1.111646)});
  vector<pair<uint64_t, double>> michael_nginx_powers({make_pair(0, 0),make_pair(59, 0.895954), make_pair(148, 8.382114), make_pair(296, 16.946102), make_pair(592, 18.640712), make_pair(1184, 19.410989), make_pair(2368, 20.412350), make_pair(2960, 20.874516), make_pair(4440, 21.798849), make_pair(5920, 21.429116), make_pair(7400, 21.886880), make_pair(8880, 22.001322), make_pair(11840, 22.010297), make_pair(14800, 21.656290), make_pair(17760, 21.505531), make_pair(20720, 21.805137), make_pair(23680, 21.928883), make_pair(26640, 22.099794), make_pair(29600, 22.133521), make_pair(44400, 21.985228), make_pair(59200, 22.510467), make_pair(74000, 22.555023), make_pair(88800, 22.379436), make_pair(118400, 22.805465), make_pair(148000, 23.444094), make_pair(177600, 23.904367), make_pair(207200, 23.900224), make_pair(236800, 23.672084), make_pair(266400, 23.197570)});
  vector<pair<uint64_t, double>> titanic_nginx_powers({make_pair(0, 0),make_pair(59, 0.000000), make_pair(148, 0.000000), make_pair(296, 3.429421), make_pair(592, 0.000000), make_pair(1184, 3.352577), make_pair(2368, 9.076655), make_pair(2960, 7.822112), make_pair(4440, 13.242242), make_pair(5920, 16.425469), make_pair(7400, 8.529208), make_pair(8880, 2.134986), make_pair(11840, 5.086099), make_pair(14800, 8.221943), make_pair(17760, 11.487016), make_pair(20720, 9.196698)});
  vector<pair<uint64_t, double>> wandboard_nginx_powers({make_pair(59, 0.321582), make_pair(148, 0.462477), make_pair(296, 0.483786), make_pair(592, 0.939856), make_pair(1184, 1.522627), make_pair(2368, 1.760076), make_pair(2960, 2.127557), make_pair(4440, 2.080671), make_pair(5920, 2.049555), make_pair(7400, 1.968275), make_pair(8880, 1.909391), make_pair(11840, 2.048343), make_pair(14800, 1.850019), make_pair(17760, 2.076675), make_pair(20720, 1.990425), make_pair(23680, 2.082291), make_pair(26640, 1.948675), make_pair(29600, 1.942293), make_pair(44400, 1.914074), make_pair(59200, 1.907416)});

  nginx_stats->SetMaxRPS("uriel", 10610);
  nginx_stats->SetMaxRPS("pandaboard", 930);
  nginx_stats->SetMaxRPS("wandboard", 2763);
  nginx_stats->SetMaxRPS("michael", 10608);
  nginx_stats->SetMaxRPS("titanic", 8222);
  nginx_stats->SetMaxRPS("gustafa", 8222);


  nginx_stats->SetPowers("uriel", uriel_nginx_powers);
  nginx_stats->SetPowers("pandaboard", pandaboard_nginx_powers);
  nginx_stats->SetPowers("wandboard", wandboard_nginx_powers);
  nginx_stats->SetPowers("michael", michael_nginx_powers);
  nginx_stats->SetPowers("gustafa", michael_nginx_powers);

  nginx_stats->SetPowers("titanic", titanic_nginx_powers);

  (*application_stats_)[TaskDescriptor::NGINX] = nginx_stats;

}

TaskType EnergyCostModel::GetTaskType(TaskID_t task_id) {
  TaskDescriptor **td = FindOrNull(*task_map_, task_id);
  CHECK_NOTNULL(td);
  return (*td)->task_type();
}

}  // namespace firmament
