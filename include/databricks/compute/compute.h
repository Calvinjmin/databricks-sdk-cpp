#pragma once

#include "databricks/core/config.h"
#include "databricks/compute/compute_types.h"

#include <string>
#include <vector>
#include <memory>

namespace databricks {
    // Forward declaration for dependency injection
    namespace internal {
        class IHttpClient;
    }

    /**
     * @brief Client for interacting with the Databricks Clusters/Compute API
     *
     * The Clusters API allows you to create, manage, and control compute clusters
     * in your Databricks workspace. This implementation uses Clusters API 2.0.
     *
     * Example usage:
     * @code
     * databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
     * databricks::Compute compute(auth);
     *
     * // List all compute clusters
     * auto cluster_list = compute.list_compute();
     *
     * // Get specific compute cluster details
     * auto cluster = compute.get_compute("1234-567890-abcde123");
     *
     * // Start a terminated compute cluster
     * compute.start_compute("1234-567890-abcde123");
     * @endcode
     */
    class Compute {
        public:
            /**
             * @brief Construct a Compute API client
             * @param auth Authentication configuration with host and token
             */
            explicit Compute(const AuthConfig& auth);

            /**
             * @brief Construct a Compute API client with dependency injection (for testing)
             * @param http_client Injected HTTP client (use MockHttpClient for unit tests)
             * @note This constructor is primarily for testing with mock HTTP clients
             */
            explicit Compute(std::shared_ptr<internal::IHttpClient> http_client);

            /**
             * @brief Destructor
             */
            ~Compute();

            // Disable copy
            Compute(const Compute&) = delete;
            Compute& operator=(const Compute&) = delete;

            /**
             * @brief List all compute clusters in the workspace
             *
             * @return Vector of Cluster objects
             * @throws std::runtime_error if the API request fails
             */
            std::vector<Cluster> list_compute();
            
            /**
             * @brief Create a new Spark Cluster 
             * 
             * @param cluster Create a cluster in Databricks with Cluster Configs
             * @return true if the operation was successful
             */
            bool create_compute(const Cluster& cluster_config);

            /**
             * @brief Get detailed information about a specific compute cluster
             *
             * @param cluster_id The unique identifier of the cluster
             * @return Cluster object with full details
             * @throws std::runtime_error if the cluster is not found or the API request fails
             */
            Cluster get_compute(const std::string& cluster_id);

            /**
             * @brief Start a terminated compute cluster
             *
             * @param cluster_id The unique identifier of the cluster to start
             * @return true if the operation was successful
             * @throws std::runtime_error if the API request fails
             */
            bool start_compute(const std::string& cluster_id);

            /**
             * @brief Terminate a running compute cluster
             *
             * @param cluster_id The unique identifier of the cluster to terminate
             * @return true if the operation was successful
             * @throws std::runtime_error if the API request fails
             *
             * @note This stops the cluster but does not permanently delete it.
             *       Terminated clusters can be restarted.
             */
            bool terminate_compute(const std::string& cluster_id);

            /**
             * @brief Restart a compute cluster
             *
             * @param cluster_id The unique identifier of the cluster to restart
             * @return true if the operation was successful
             * @throws std::runtime_error if the API request fails
             *
             * @note This will terminate and then start the cluster with the same configuration.
             */
            bool restart_compute(const std::string& cluster_id);

        private:
            class Impl;
            std::unique_ptr<Impl> pimpl_;

            bool compute_operation(const std::string& cluster_id, const std::string& endpoint, const std::string& operation_name);
            static std::vector<Cluster> parse_compute_list(const std::string& json_str);
            static Cluster parse_compute(const std::string& json_str);
    };
} // namespace databricks