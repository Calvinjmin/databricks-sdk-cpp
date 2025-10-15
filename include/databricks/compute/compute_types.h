#pragma once

#include <string>
#include <map>

namespace databricks {
    enum class ClusterStateEnum {
        PENDING,
        RUNNING,
        RESTARTING,
        RESIZING,
        TERMINATING,
        TERMINATED,
        ERROR,
        UNKNOWN  
    };

    // Helper functions for string conversion
    ClusterStateEnum parse_cluster_state(const std::string& state_str);
    std::string cluster_state_to_string(ClusterStateEnum state);

    struct Cluster {
        std::string cluster_id;
        std::string cluster_name;
        std::string state;
        std::string creater_user_name;
        uint64_t start_time = 0;
        uint64_t terminated_time = 0;
        std::string spark_version;
        std::string node_type_id;
        int num_workers = 0;
        std::map<std::string, std::string> custom_tags;
    }; 

    struct ClusterState {
        std::string cluster_id;
        ClusterStateEnum cluster_state;
        std::string state_message;

        static ClusterState from_json(const std::string& json_str);
    };

} // namespace databricks 