#include "databricks/compute/compute_types.h"
#include <algorithm>
#include <cctype>

namespace databricks {

ClusterStateEnum parse_cluster_state(const std::string& state_str) {
    // Convert to uppercase for case-insensitive comparison
    std::string upper_state = state_str;
    std::transform(upper_state.begin(), upper_state.end(), upper_state.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (upper_state == "PENDING") return ClusterStateEnum::PENDING;
    if (upper_state == "RUNNING") return ClusterStateEnum::RUNNING;
    if (upper_state == "RESTARTING") return ClusterStateEnum::RESTARTING;
    if (upper_state == "RESIZING") return ClusterStateEnum::RESIZING;
    if (upper_state == "TERMINATING") return ClusterStateEnum::TERMINATING;
    if (upper_state == "TERMINATED") return ClusterStateEnum::TERMINATED;
    if (upper_state == "ERROR") return ClusterStateEnum::ERROR;

    return ClusterStateEnum::UNKNOWN;
}

std::string cluster_state_to_string(ClusterStateEnum state) {
    switch (state) {
        case ClusterStateEnum::PENDING:
            return "PENDING";
        case ClusterStateEnum::RUNNING:
            return "RUNNING";
        case ClusterStateEnum::RESTARTING:
            return "RESTARTING";
        case ClusterStateEnum::RESIZING:
            return "RESIZING";
        case ClusterStateEnum::TERMINATING:
            return "TERMINATING";
        case ClusterStateEnum::TERMINATED:
            return "TERMINATED";
        case ClusterStateEnum::ERROR:
            return "ERROR";
        case ClusterStateEnum::UNKNOWN:
            return "UNKNOWN";
        default:
            return "UNKNOWN";
    }
}

} // namespace databricks
