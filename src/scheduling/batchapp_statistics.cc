#include "scheduling/batchapp_statistics.h"

#include "misc/utils.h"
#include "misc/map-util.h"

#include <algorithm>
#include <math.h>
#include <limits>

namespace firmament {



BatchAppStatistics::BatchAppStatistics() :
  machine_to_power_(new MachinePowerMap()),
  machine_to_runtime_(new MachineRuntimeMap())
  //machine_to_completion_stats_(new unordered_map<string, CompletionStatistics>())
 {
  power_stats_.max_stat = 0;
  power_stats_.min_stat = numeric_limits<double>::max();

  runtime_stats_.max_stat = 0;
  runtime_stats_.min_stat = numeric_limits<double>::max();
}

  double BatchAppStatistics::GetPower(string machine) {
    return (*machine_to_power_)[machine];
  }

  double BatchAppStatistics::GetRunningRuntime(string machine, uint64_t units, double completed) {
    UnitRuntimeMap *unit_runtime_map = FindPtrOrNull(*machine_to_runtime_,machine);// size);
    // TODO allow for non-defined values.

    CHECK_NOTNULL(unit_runtime_map);
    /*
      map::equal::range
      Returns the bounds of a range that includes all the elements in the container which
      have a key equivalent to k. Because the elements in a map container have unique keys,
      the range returned will contain a single element at most.

      If no matches are found, the range returned has a length of zero, with both iterators pointing
      to the first element that has  a key considered to go after k according to the
      container's internal comparison object (key_comp).
    */
    pair<UnitRuntimeMap::iterator, UnitRuntimeMap::iterator> ret =
      unit_runtime_map->equal_range(units);

    double runtime;

    if (ret.first != ret.second) {
      // FOUND! use the exact value.
      runtime = ret.first->second;
    } else {
      // NOT FOUND, do linear interpolation between the two closest points.
      // We assume we will only ever see values s.t. min elem <= elem <= max elem
      // i.e. nothing smaller than min or bigger than max.
      CHECK(ret.first != unit_runtime_map->begin());
      CHECK(ret.second != unit_runtime_map->end());
      --ret.first;
      // Now first points to the element before our insertion point and second to element after
      // insertion point.

      double frac_left = (units - ret.first->first) / double(ret.second->first - ret.first->first);
      double frac_right = 1 - frac_left;
      runtime = frac_left * ret.first->second + frac_right * ret.second->second;
    }

    return runtime * (1 - completed);
  }

  double BatchAppStatistics::GetRuntime(string machine, uint64_t size) {
    return GetRunningRuntime(machine, size, 0);
  }

  void BatchAppStatistics::SetRuntime(string machine, uint64_t size, double runtime) {


  }

  // TODO allow for different power levels?
  void BatchAppStatistics::SetPower(string machine, double power) {
    (*machine_to_power_)[machine] = power;
  }


  void BatchAppStatistics::SetRuntimes(string machine, vector<pair<uint64_t, double>> units_runtimes) {
    UnitRuntimeMap *unit_runtime_map = FindPtrOrNull(*machine_to_runtime_,machine);
    if (!unit_runtime_map) {
      unit_runtime_map = new UnitRuntimeMap();
      (*machine_to_runtime_)[machine] = unit_runtime_map;
    }

    for (auto &units_runtime : units_runtimes) {
      (*unit_runtime_map)[units_runtime.first] = units_runtime.second;
    }
  }

// // Get energy for the given machine with a fraction completed.
// double BatchAppStatistics::GetEnergy(string machine, double completed) {
//   // BUG sideeffects of accessor..
//   return (*machine_to_energy_)[machine] * (1 - completed);
// }

// void BatchAppStatistics::SetEnergy(string machine, double energy) {
//   // TODO consider whether we want energy / s or total energy here!
//   SetStat(*machine_to_energy_, power_stats_, machine, energy);
// }

// // Get Runtime for the given machine with a fraction completed.
// double BatchAppStatistics::GetRuntime(string machine, double completed) {
//   return (*machine_to_runtime_)[machine] * (1 - completed);
// }

// void BatchAppStatistics::SetRuntime(string machine, double runtime) {
//   SetStat(*machine_to_runtime_, runtime_stats_, machine, runtime);
// }

// double BatchAppStatistics::BestEnergy(uint64_t units) {
//   return power_stats_.min_stat * pow(units, energy_multiplier_) * ;
// }

// double BatchAppStatistics::WorstEnergy(uint64_t units) {
//   return power_stats_.max_stat;
// }

// double BatchAppStatistics::BestRuntime() {
//   return runtime_stats_.min_stat;
// }

// double BatchAppStatistics::WorstRuntime() {
//   return runtime_stats_.max_stat;
// }


bool BatchAppStatistics::HasStatistics() {
  return machine_to_runtime_->size() != 0;
}

void BatchAppStatistics::PrintStats() {
// TODO implement
}

}
 // namespace firmament