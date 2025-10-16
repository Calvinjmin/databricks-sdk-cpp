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
    std::vector<Cluster> Compute::list_compute() {
        internal::get_logger()->info("Listing compute clusters");

        // Make API request
        auto response = pimpl_->http_client_->get("/clusters/list");
        pimpl_->http_client_->check_response(response, "listCompute");

        internal::get_logger()->debug("Compute clusters list response: " + response.body);
        return parse_compute_list(response.body);
    }

    Cluster Compute::get_compute(const std::string& cluster_id){
        internal::get_logger()->info("Getting compute cluster details for cluster_id=" + cluster_id);

        // Make API request with cluster_id as query parameter
        auto response = pimpl_->http_client_->get("/clusters/get?cluster_id=" + cluster_id);
        pimpl_->http_client_->check_response(response, "getCompute");

        internal::get_logger()->debug("Compute cluster details response: " + response.body);
        return parse_compute(response.body);
    }

    bool Compute::compute_operation(const std::string& cluster_id, const std::string& endpoint, const std::string& operation_name) {
        internal::get_logger()->info(operation_name + " compute cluster id=" + cluster_id);

        json body_json;
        body_json["cluster_id"] = cluster_id;
        std::string body = body_json.dump();

        internal::get_logger()->debug(operation_name + " request body: " + body);

        // API Request
        auto response = pimpl_->http_client_->post(endpoint, body);
        pimpl_->http_client_->check_response(response, operation_name);

        internal::get_logger()->info("Successfully " + operation_name + " compute cluster id=" + cluster_id);
        return true;
    }

    bool Compute::start_compute(const std::string& cluster_id) {
        return compute_operation(cluster_id, "/clusters/start", "startCompute");
    }

    bool Compute::terminate_compute(const std::string& cluster_id) {
        return compute_operation(cluster_id, "/clusters/delete", "terminateCompute");
    }

    bool Compute::restart_compute(const std::string& cluster_id) {
        return compute_operation(cluster_id, "/clusters/restart", "restartCompute");
    }

    // Private Methods
    std::vector<Cluster> Compute::parse_compute_list(const std::string& json_str) {
        std::vector<Cluster> clusters;

        try {
            auto j = json::parse(json_str);

            if (!j.contains("clusters") || !j["clusters"].is_array()) {
                internal::get_logger()->warn("No clusters array found in response");
                return clusters;
            }

            for (const auto& cluster_json : j["clusters"]) {
                clusters.push_back(parse_compute(cluster_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(clusters.size()) + " compute clusters");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse compute clusters list: " +
        std::string(e.what()));
            throw std::runtime_error("Failed to parse compute clusters list: " + std::string(e.what()));
        }

        return clusters;
    }

    Cluster Compute::parse_compute(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            Cluster cluster;

            cluster.cluster_id = j.value("cluster_id", "");
            cluster.cluster_name = j.value("cluster_name", "");
            cluster.state = j.value("state", "");
            cluster.creator_user_name = j.value("creator_user_name", "");
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
            throw std::runtime_error("Failed to parse Compute Cluster JSON: " + std::string(e.what()));
        }
    }
    
} // namespace databricks 
