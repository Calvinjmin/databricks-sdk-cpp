#pragma once

#include "databricks/core/config.h"
#include "databricks/compute/compute_types.h"

namespace databricks {
    class Compute {
        public:
            explicit Compute(const AuthConfig& auth);
            ~Compute();

            std::vector<Cluster> list_clusters();

            Cluster get_cluster( const std::string& cluster_id );
            bool start_cluster( const std::string& cluster_id );
            bool terminate_cluster( const std::string& cluster_id );
            bool restart_cluster( const std::string& cluster_id );

        private:
            class Impl;
            std::unique_ptr<Impl> pimpl_;

            static std::vector<Cluster> parse_clusters_list( const std::string& json_str );
            static Cluster parse_cluster(const std::string& json_str );
    };
} // namespace databricks