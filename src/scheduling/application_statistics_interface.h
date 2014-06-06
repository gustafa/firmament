// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
//

#ifndef FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H
#define FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H

#include "base/types.h"

namespace firmament {


class ApplicationStatistics {

struct MachineAppStat {
  string min_machine;
  string max_machine;
  double min_stat;
  double max_stat;
};


public:
  ApplicationStatistics() {};
  virtual ~ApplicationStatistics() {};


  virtual void PrintStats() = 0;

};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H