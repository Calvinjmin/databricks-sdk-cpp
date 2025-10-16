#pragma once

#include <string>
#include <map>
#include <cstdint>

namespace databricks {

    /**
     * @brief Enumeration of possible cluster lifecycle states
     *
     * Represents the various states a Databricks cluster can be in during its lifecycle.
     */
    enum class ClusterStateEnum {
        PENDING,      ///< Cluster is being created
        RUNNING,      ///< Cluster is running and ready for use
        RESTARTING,   ///< Cluster is restarting
        RESIZING,     ///< Cluster is being resized
        TERMINATING,  ///< Cluster is being terminated
        TERMINATED,   ///< Cluster has been terminated
        ERROR,        ///< Cluster is in an error state
        UNKNOWN       ///< Unknown or unrecognized state
    };

    /**
     * @brief Parse a cluster state string into ClusterStateEnum
     * @param state_str String representation of the cluster state
     * @return ClusterStateEnum corresponding to the string
     */
    ClusterStateEnum parse_cluster_state(const std::string& state_str);

    /**
     * @brief Convert ClusterStateEnum to string representation
     * @param state ClusterStateEnum value
     * @return String representation of the state
     */
    std::string cluster_state_to_string(ClusterStateEnum state);

    /**
     * @brief Represents a Databricks cluster
     *
     * Clusters are compute resources used to run notebooks, jobs, and other workloads.
     * This struct contains the core metadata about a cluster configuration and its state.
     */
    struct Cluster {
        std::string cluster_id;                      ///< Unique identifier for the cluster
        std::string cluster_name;                    ///< Display name of the cluster
        std::string state;                           ///< Current lifecycle state (e.g., "RUNNING", "TERMINATED")
        std::string creator_user_name;               ///< Username of the cluster creator
        uint64_t start_time = 0;                     ///< Unix timestamp in milliseconds when cluster started
        uint64_t terminated_time = 0;                ///< Unix timestamp in milliseconds when cluster terminated (0 if running)
        std::string spark_version;                   ///< Spark runtime version (e.g., "11.3.x-scala2.12")
        std::string node_type_id;                    ///< Cloud provider instance type (e.g., "i3.xlarge")
        int num_workers = 0;                         ///< Number of worker nodes in the cluster
        std::map<std::string, std::string> custom_tags; ///< User-defined tags for organization and tracking
    };

    /**
     * @brief Represents detailed state information for a cluster
     *
     * Provides more granular state information including state messages
     * that can help diagnose issues or understand cluster status.
     */
    struct ClusterState {
        std::string cluster_id;                              ///< Unique identifier for the cluster
        ClusterStateEnum cluster_state = ClusterStateEnum::UNKNOWN;  ///< Enumerated state value (default: UNKNOWN)
        std::string state_message;                           ///< Human-readable message describing the state

        /**
         * @brief Parse a ClusterState from JSON string
         * @param json_str JSON representation of cluster state
         * @return Parsed ClusterState object
         * @throws std::runtime_error if parsing fails
         */
        static ClusterState from_json(const std::string& json_str);
    };

} // namespace databricks 