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

public:
  enum APPLICATION_TYPE {
    REAL_TIME,
    BATCH
  };

  ApplicationStatistics(APPLICATION_TYPE type, double alone_delta_j, uint64_t alone_runtime_stat);

  uint64_t GetExpectedEnergyUse();

  uint64_t GetExpectedRuntime();

  void SetDeltaJ(double delta_j);

  void SetExpectedRuntime(uint64_t runtime);

  const APPLICATION_TYPE application_type;

 private:
  double alone_delta_j_;

  // Operations / s if a real-time application or runtime in the case of a batch application.
  uint64_t alone_runtime_stat_;
};

} // namespace firmament

#endif // FIRMAMENT_SCHEDULING_APPLICATION_STATISTICS_H