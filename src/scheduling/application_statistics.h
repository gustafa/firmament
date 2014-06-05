// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
//

#ifndef FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H

#include "base/types.h"
#include "scheduling/completion_statistics.h"

namespace firmament {


class ApplicationStatistics {
  // Runtime and energy statistics for an application (on a given host).
// TODO use real MB instead of "units"

typedef map<uint64_t, double> UnitRuntimeMap;
typedef unordered_map<string, UnitRuntimeMap> MachineRuntimeMap;
typedef unordered_map<string, double> MachinePowerMap;


struct MachineAppStat {
  string min_machine;
  string max_machine;
  double min_stat;
  double max_stat;
};


public:
  ApplicationStatistics();

  double GetRunningEnergy(string machine, uint64_t size, double completed);
  double GetEnergy(string machine, double runtime);

  double GetRunningRuntime(string machine, uint64_t size, double completed);
  double GetRuntime(string machine, uint64_t size);

  void SetRuntime(string machine, uint64_t size, double runtime);

  // TODO allow for different power levels?
  void SetPower(string machine, double energy);


  // Returns whether we have observable data.
  bool HasStatistics();

  void PrintStats();

 private:

  double power_;

  MachineAppStat power_stats_;
  MachineAppStat runtime_stats_;

  shared_ptr<MachinePowerMap> machine_to_power_;

  shared_ptr<MachineRuntimeMap> machine_to_runtime_;


  // What is this for?
  shared_ptr<unordered_map<string, CompletionStatistics>> machine_to_completion_stats_;



};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H