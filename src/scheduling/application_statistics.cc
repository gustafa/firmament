#include "scheduling/application_statistics.h"

#include "misc/utils.h"
#include "misc/map-util.h"

#include <limits>

namespace firmament {

ApplicationStatistics::ApplicationStatistics() {

  machine_to_energy_.reset(new MachineStatMap());
  machine_to_runtime_.reset(new MachineStatMap());

  energy_stats_.max_stat = 0;
  energy_stats_.min_stat = numeric_limits<double>::max();

  runtime_stats_.max_stat = 0;
  runtime_stats_.min_stat = numeric_limits<double>::max();
}

// Get energy for the given machine with a fraction completed.
double ApplicationStatistics::GetEnergy(string machine, double completed) {
  // BUG sideeffects of accessor..
  return (*machine_to_energy_)[machine] * (1 - completed);
}

void ApplicationStatistics::SetEnergy(string machine, double energy) {
  // TODO consider whether we want energy / s or total energy here!
  SetStat(*machine_to_energy_, energy_stats_, machine, energy);
}

// Get Runtime for the given machine with a fraction completed.
double ApplicationStatistics::GetRuntime(string machine, double completed) {
  return (*machine_to_runtime_)[machine] * (1 - completed);
}

void ApplicationStatistics::SetRuntime(string machine, double runtime) {
  SetStat(*machine_to_runtime_, runtime_stats_, machine, runtime);
}

double ApplicationStatistics::BestEnergy() {
  return energy_stats_.min_stat;
}

double ApplicationStatistics::WorstEnergy() {
  return energy_stats_.max_stat;
}

double ApplicationStatistics::BestRuntime() {
  return runtime_stats_.min_stat;
}

double ApplicationStatistics::WorstRuntime() {
  return runtime_stats_.max_stat;
}


bool ApplicationStatistics::HasStatistics() {
  return machine_to_energy_->size() != 0;
}


void ApplicationStatistics::SetStat(MachineStatMap &stat_map, MachineAppStat &stats,
                                    string &machine, double stat) {
      // Update the value of the hashmap.
  stat_map[machine] = stat;
  // Check if we are to upset the stat statuses, and if so recompute the stat.
  if ((stats.min_machine == machine && stat > stats.min_stat) ||
      (stats.max_machine == machine && stat < stats.max_stat)) {
    RecomputeStats(stat_map, stats);
  } else {
    // Otherwise check if we are going above max or below min and update accordingly.
    if (stat < stats.min_stat) {
      stats.min_stat = stat;
      stats.min_machine = machine;
    }
    if (stat > stats.max_stat) {
      stats.max_stat = stat;
      stats.max_machine = machine;
    }
  }
}

void ApplicationStatistics::RecomputeStats(MachineStatMap &stat_map, MachineAppStat &stats) {
  // Only recompute if statistics are available.
  if (stat_map.size()) {
    auto it = stat_map.begin();
    // Set initial values to the first element and then iterate over the rest.
    // it is a (hostname, stat) pair.
    stats.max_machine = it->first;
    stats.min_machine = it->first;
    stats.min_stat = it->second;
    stats.max_stat = it->second;
    ++it;
    for (; it != stat_map.end(); ++it) {
      if (it->second < stats.min_stat) {
        stats.min_stat = it->second;
        stats.min_machine = it->first;
      } else if (it->second > stats.max_stat) {
        stats.max_stat = it->second;
        stats.max_machine = it->first;
      }
    }
  }
}

}
 // namespace firmament