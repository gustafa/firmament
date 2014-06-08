#include "scheduling/serviceapp_statistics.h"

#include "misc/utils.h"
#include "misc/map-util.h"

#include <algorithm>
#include <math.h>
#include <limits>

namespace firmament {



ServiceAppStatistics::ServiceAppStatistics() :
  machine_to_power_(new MachinePowerMap()),
  machine_to_max_rps_(new MachineRPSMap()) {
  }

  uint64_t ServiceAppStatistics::MaxRPS(string hostname) {
    uint64_t *rps = FindOrNull(*machine_to_max_rps_, hostname);

    CHECK_NOTNULL(rps);
    return *rps;
  }

  void ServiceAppStatistics::SetMaxRPS(string hostname, uint64_t rps) {
    (*machine_to_max_rps_)[hostname] = rps;
  }

  double ServiceAppStatistics::GetPower(string machine, uint64_t units) {
    // TODO clean this up!
    // TODO allow for non-defined values.

     UnitPowerMap *unit_power_map = FindPtrOrNull(*machine_to_power_, machine);// size);

    CHECK_NOTNULL(unit_power_map);
    /*
      map::equal::range
      Returns the bounds of a range that includes all the elements in the container which
      have a key equivalent to k. Because the elements in a map container have unique keys,
      the range returned will contain a single element at most.

      If no matches are found, the range returned has a length of zero, with both iterators pointing
      to the first element that has  a key considered to go after k according to the
      container's internal comparison object (key_comp).
    */
    pair<UnitPowerMap::iterator, UnitPowerMap::iterator> ret =
      unit_power_map->equal_range(units);

    double runtime;

    if (ret.first != ret.second) {
      // FOUND! use the exact value.
      runtime = ret.first->second;
    } else {
      // NOT FOUND, do linear interpolation between the two closest points.
      // We assume we will only ever see values s.t. min elem <= elem <= max elem
      // i.e. nothing smaller than min or bigger than max.
      CHECK(ret.first != unit_power_map->begin());
      CHECK(ret.second != unit_power_map->end());
      --ret.first;
      // Now first points to the element before our insertion point and second to element after
      // insertion point.

      double frac_left = (units - ret.first->first) / double(ret.second->first - ret.first->first);
      double frac_right = 1 - frac_left;
      runtime = frac_left * ret.first->second + frac_right * ret.second->second;
    }

    return runtime;
  }

  void ServiceAppStatistics::SetPowers(string machine, vector<pair<uint64_t, double>> units_powers) {
    UnitPowerMap *unit_power_map = FindPtrOrNull(*machine_to_power_,machine);
    if (!unit_power_map) {
      unit_power_map = new UnitPowerMap();
      (*machine_to_power_)[machine] = unit_power_map;
    }

    for (auto &unit_power : units_powers) {
      (*unit_power_map)[unit_power.first] = unit_power.second;
    }
  }

void ServiceAppStatistics::PrintStats() {
// TODO implement
}


}
 // namespace firmament