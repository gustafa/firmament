// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
//

#ifndef FIRMAMENT_SCHEDULING_BATCH_APPLICATION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_BATCH_APPLICATION_STATISTICS_H

#include "scheduling/application_statistics_interface.h"

#include "base/types.h"
#include "scheduling/completion_statistics.h"

namespace firmament {


class BatchAppStatistics : public ApplicationStatistics {
  // Runtime and energy statistics for an application (on a given host).
// TODO use real MB instead of "units"

typedef map<uint64_t, double> UnitRuntimeMap;
typedef unordered_map<string, UnitRuntimeMap *> MachineRuntimeMap;
typedef unordered_map<string, double> MachinePowerMap;


struct MachineAppStat {
  string min_machine;
  string max_machine;
  double min_stat;
  double max_stat;
};


public:
  BatchAppStatistics();

  double GetRunningEnergy(string machine, uint64_t size, double completed);
  double GetPower(string machine);

  double GetRunningRuntime(string machine, uint64_t size, double completed);
  double GetRuntime(string machine, uint64_t size);

  void SetRuntime(string machine, uint64_t size, double runtime);

  // TODO allow for different power levels?
  void SetPower(string machine, double energy);

  void SetRuntimes(string machine, vector<pair<uint64_t, double>> units_runtimes);


  // Returns whether we have observable data.
  bool HasStatistics();

  void PrintStats();

 private:

  MachineAppStat power_stats_;
  MachineAppStat runtime_stats_;

  shared_ptr<MachinePowerMap> machine_to_power_;
  shared_ptr<MachineRuntimeMap> machine_to_runtime_;

  // What is this for?
  shared_ptr<unordered_map<string, CompletionStatistics>> machine_to_completion_stats_;

};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_BATCH_APPLICATION_STATISTICS_H