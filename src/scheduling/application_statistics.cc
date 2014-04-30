#include "scheduling/application_statistics.h"

namespace firmament {

ApplicationStatistics::ApplicationStatistics(APPLICATION_TYPE type, double alone_delta_j,
                                             uint64_t alone_runtime_stat)
  : application_type(type),
    alone_delta_j_(alone_delta_j),
    alone_runtime_stat_(alone_runtime_stat) {
}


} // namespace firmament