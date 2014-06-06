// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
//

#ifndef FIRMAMENT_SCHEDULING_SERVICE_APPLICATION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_SERVICE_APPLICATION_STATISTICS_H

#include "scheduling/application_statistics_interface.h"

#include "base/types.h"
#include "scheduling/completion_statistics.h"

namespace firmament {


class ServiceAppStatistics : public ApplicationStatistics {
  // Runtime and energy statistics for an application (on a given host).
// TODO use real MB instead of "units"

typedef map<uint64_t, double> UnitPowerMap;

typedef unordered_map<string, UnitPowerMap *> MachinePowerMap;
typedef unordered_map<string, uint64_t> MachineRPSMap;



public:
  ServiceAppStatistics();
  void SetPowers(string machine, vector<pair<uint64_t, double>> unit_rps);

  uint64_t MaxRPS(string hostname);
  void SetMaxRPS(string hostname, uint64_t rps);

  double GetPower(string hostname, uint64_t units);

  void PrintStats();


 private:
  shared_ptr<MachinePowerMap> machine_to_power_;
  shared_ptr<MachineRPSMap> machine_to_max_rps_;
};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_SERVICE_APPLICATION_STATISTICS_H