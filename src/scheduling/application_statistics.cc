#include "scheduling/application_statistics.h"

namespace firmament {

ApplicationStatistics::ApplicationStatistics(APPLICATION_TYPE type, double alone_delta_j,
                                             uint64_t alone_runtime_stat)
  : application_type(type),
    alone_delta_j_(alone_delta_j),
    alone_runtime_stat_(alone_runtime_stat) {
}


// Returns a x 100 rounded number of the energy cost.
uint64_t ApplicationStatistics::GetExpectedEnergyUse() {
  return alone_delta_j_ * alone_runtime_stat_ * 100;
}

uint64_t ApplicationStatistics::GetExpectedRuntime() {
  return alone_runtime_stat_;
}


void ApplicationStatistics::SetDeltaJ(double delta_j) {
  alone_delta_j_ = delta_j;
}

void ApplicationStatistics::SetExpectedRuntime(uint64_t runtime){
  alone_runtime_stat_ = runtime;
}


} // namespace firmament