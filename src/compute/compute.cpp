#include "databricks/compute/compute.h"
#include "../internal/http_client.h"
#include "../internal/logger.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace databricks {
    class Compute::Impl {
        public:
            explicit Impl(const AuthConfig& auth)
                : http_client_( std::make_unique<internal::HttpClient>(auth) ){}

            std::unique_ptr<internal::HttpClient> http_client_;
    };

    Compute::Compute(const AuthConfig& auth)
        : pimpl_(std::make_unique<Impl>(auth)) {}
    
    Compute::~Compute() = default;

    // Public Methods
    std::vector<Cluster> Compute::list_clusters() {
        internal::get_logger()->info("Listing clusters");

        // Make API request
        auto response = pimpl_->http_client_->get("/clusters/list");

        if (response.status_code != 200) {
            std::string error_msg = "Failed to list clusters: HTTP " +
                                    std::to_string(response.status_code) +
                                    " - " + response.body;
            internal::get_logger()->error(error_msg);
            throw std::runtime_error(error_msg);
        }

        internal::get_logger()->debug("Clusters list response: " + response.body);
        return parse_clusters_list(response.body);
    }

    Cluster Compute::get_cluster(const std::string& cluster_id ){
        internal::get_logger()->info("Getting cluster details for cluster_id=" + cluster_id);

        // Make API request with cluster_id as query parameter
        auto response = pimpl_->http_client_->get("/clusters/get?cluster_id=" + cluster_id);

        if (response.status_code != 200) {
            std::string error_msg = "Failed to get cluster: HTTP " +
                                    std::to_string(response.status_code) +
                                    " - " + response.body;
            internal::get_logger()->error(error_msg);
            throw std::runtime_error(error_msg);
        }

        internal::get_logger()->debug("Cluster details response: " + response.body);
        return parse_cluster(response.body);
    }

    bool Compute::start_cluster(const std::string& cluster_id) {
        return true;
    }

    bool Compute::terminate_cluster(const std::string& cluster_id) {
        return false;
    }

    bool Compute::restart_cluster(const std::string& cluster_id) {
        return false;
    }

    // Private Methods
    std::vector<Cluster> Compute::parse_clusters_list(const std::string& json_str ) {
        std::vector<Cluster> clusters;

        try {
            auto j = json::parse(json_str);

            if (!j.contains("clusters") || !j["clusters"].is_array()) {
                internal::get_logger()->warn("No clusters array found in response");
                return clusters;
            }

            for (const auto& cluster_json : j["clusters"]) {
                clusters.push_back(parse_cluster(cluster_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(clusters.size()) + " clusters");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse clusters list: " +
        std::string(e.what()));
            throw std::runtime_error("Failed to parse clusters list: " + std::string(e.what()));
        }

        return clusters;
    }

    Cluster Compute::parse_cluster(const std::string& json_str ) {
        try {
            auto j = json::parse(json_str);
            Cluster cluster;

            cluster.cluster_id = j.value("cluster_id", "");
            cluster.cluster_name = j.value("cluster_name", "");
            cluster.state = j.value("state", "");
            cluster.creater_user_name = j.value("creator_user_name", "");
            cluster.start_time = j.value("start_time", uint64_t(0));
            cluster.terminated_time = j.value("terminated_time", uint64_t(0));
            cluster.spark_version = j.value("spark_version", "");
            cluster.node_type_id = j.value("node_type_id", "");
            cluster.num_workers = j.value("num_workers", 0);

            // Parse custom tags if present
            if (j.contains("custom_tags") && j["custom_tags"].is_object()) {
                for (auto& [key, value] : j["custom_tags"].items()) {
                    if (value.is_string()) {
                        cluster.custom_tags[key] = value.get<std::string>();
                    }
                }
            }

            return cluster;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Cluster JSON: " + std::string(e.what()));
        }
    }
    
} // namespace databricks 
