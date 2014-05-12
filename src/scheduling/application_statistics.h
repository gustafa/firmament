// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
//

#ifndef FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H

#include "base/types.h"

namespace firmament {

class ApplicationStatistics {
  // Runtime and energy statistics for an application (on a given host).

typedef unordered_map<string, double> MachineStatMap;


struct MachineAppStat {
  string min_machine;
  string max_machine;
  double min_stat;
  double max_stat;
};

public:
  ApplicationStatistics();

  double GetEnergy(string machine, double completed);
  void SetEnergy(string machine, double energy);

  double GetRuntime(string machine, double completed);
  void SetRuntime(string machine, double runtime);

  double BestEnergy();
  double WorstEnergy();

  double BestRuntime();
  double WorstRuntime();

  double GetExpectedEnergyUse();
  double GetExpectedRuntime();

  // Returns whether we have observable data.
  bool HasStatistics();

 private:

  MachineAppStat energy_stats_;
  MachineAppStat runtime_stats_;

  shared_ptr<MachineStatMap> machine_to_energy_;
  shared_ptr<MachineStatMap> machine_to_runtime_;

  void SetStat(MachineStatMap &stat_map, MachineAppStat &stats,
              string &machine, double stat);

  void RecomputeStats(MachineStatMap &stat_map, MachineAppStat &stats);


};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H