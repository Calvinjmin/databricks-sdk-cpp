// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
/**
 * @file compute_example.cpp
 * @brief Example demonstrating the Databricks Compute/Clusters API
 *
 * This example shows how to:
 * 1. List all compute clusters in your workspace
 * 2. Get details for a specific cluster
 * 3. Create a new compute cluster
 * 4. Manage cluster lifecycle (start, restart, terminate)
 *
 * Configuration:
 * - Set DATABRICKS_HOST and DATABRICKS_TOKEN environment variables
 * - Or create ~/.databrickscfg with [DEFAULT] section
 *
 * Note: Creating clusters will incur costs. This example creates a minimal
 * single-node cluster to minimize costs, but please terminate it when done.
 */

#include "databricks/compute/compute.h"
#include "databricks/core/config.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

// Helper function to print cluster details
void print_cluster_info(const databricks::Cluster& cluster, const std::string& indent = "  ") {
    std::cout << indent << "Cluster ID:      " << cluster.cluster_id << std::endl;
    std::cout << indent << "Name:            " << cluster.cluster_name << std::endl;
    std::cout << indent << "State:           " << cluster.state << std::endl;
    std::cout << indent << "Creator:         " << cluster.creator_user_name << std::endl;
    std::cout << indent << "Spark Version:   " << cluster.spark_version << std::endl;
    std::cout << indent << "Node Type:       " << cluster.node_type_id << std::endl;
    std::cout << indent << "Num Workers:     " << cluster.num_workers;
    if (cluster.num_workers == 0) {
        std::cout << " (Single-node mode)";
    }
    std::cout << std::endl;

    if (!cluster.custom_tags.empty()) {
        std::cout << indent << "Custom Tags:" << std::endl;
        for (const auto& [key, value] : cluster.custom_tags) {
            std::cout << indent << "  - " << key << ": " << value << std::endl;
        }
    }

    if (cluster.start_time > 0) {
        std::cout << indent << "Start Time:      " << cluster.start_time << std::endl;
    }
    if (cluster.terminated_time > 0) {
        std::cout << indent << "Terminated Time: " << cluster.terminated_time << std::endl;
    }
}

// Helper function to wait for cluster state
bool wait_for_cluster_state(databricks::Compute& compute, const std::string& cluster_id,
                            const std::string& target_state, int max_attempts = 60, int wait_seconds = 10) {
    std::cout << "\nWaiting for cluster to reach state: " << target_state << std::endl;

    for (int i = 0; i < max_attempts; i++) {
        auto cluster = compute.get_compute(cluster_id);
        std::cout << "  Current state: " << cluster.state << " (" << (i + 1) << "/" << max_attempts << ")" << std::endl;

        if (cluster.state == target_state) {
            std::cout << "  Cluster reached target state: " << target_state << std::endl;
            return true;
        }

        // Check for error states
        if (cluster.state == "ERROR" || cluster.state == "TERMINATING") {
            std::cout << "  Cluster entered error/terminating state" << std::endl;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));
    }

    std::cout << "  Timeout waiting for cluster to reach state: " << target_state << std::endl;
    return false;
}

int main() {
    try {
        // Load configuration from environment or ~/.databrickscfg
        databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

        std::cout << "========================================" << std::endl;
        std::cout << "Databricks Compute API Example" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Connecting to: " << auth.host << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Create Compute API client
        databricks::Compute compute(auth);

        // ===================================================================
        // Example 1: List all compute clusters
        // ===================================================================
        std::cout << "1. Listing all compute clusters:" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        auto clusters = compute.list_compute();
        std::cout << "Found " << clusters.size() << " cluster(s):\n" << std::endl;

        for (const auto& cluster : clusters) {
            print_cluster_info(cluster);
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 2: Get details for a specific cluster
        // ===================================================================
        if (!clusters.empty()) {
            std::string first_cluster_id = clusters[0].cluster_id;

            std::cout << "\n2. Getting details for cluster " << first_cluster_id << ":" << std::endl;
            std::cout << "-----------------------------------------------------" << std::endl;

            auto cluster_details = compute.get_compute(first_cluster_id);
            print_cluster_info(cluster_details);
        }

        // ===================================================================
        // Example 3: Create a new compute cluster
        // ===================================================================
        std::cout << "\n3. Creating a new compute cluster:" << std::endl;
        std::cout << "----------------------------------" << std::endl;

        std::cout << "\nWARNING: This will create a cluster and may incur costs!" << std::endl;
        std::cout << "Press Enter to continue or Ctrl+C to cancel..." << std::endl;
        std::cin.get();

        // Create a minimal single-node cluster to minimize costs
        databricks::Cluster cluster_config;
        cluster_config.cluster_name = "sdk-example-cluster";
        cluster_config.spark_version = "11.3.x-scala2.12"; // Use a stable version
        cluster_config.node_type_id = "i3.xlarge";         // AWS instance type (adjust for your cloud)
        cluster_config.num_workers = 0;                    // Single-node mode (driver only)

        // Add custom tags for tracking
        cluster_config.custom_tags["created_by"] = "databricks-cpp-sdk";
        cluster_config.custom_tags["purpose"] = "example";
        cluster_config.custom_tags["auto_delete"] = "true";

        std::cout << "\nCluster configuration:" << std::endl;
        print_cluster_info(cluster_config);

        std::cout << "\nCreating cluster..." << std::endl;
        bool created = compute.create_compute(cluster_config);

        if (created) {
            std::cout << "\n✓ Cluster creation initiated successfully!" << std::endl;
            std::cout << "\nNote: The cluster is now being created. You can check its status" << std::endl;
            std::cout << "in the Databricks UI or by listing clusters again." << std::endl;

            // Find the newly created cluster by name
            std::cout << "\nSearching for newly created cluster..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait a bit for API to update

            auto updated_clusters = compute.list_compute();
            std::string new_cluster_id;

            for (const auto& cluster : updated_clusters) {
                if (cluster.cluster_name == cluster_config.cluster_name) {
                    new_cluster_id = cluster.cluster_id;
                    std::cout << "Found cluster: " << new_cluster_id << std::endl;
                    std::cout << "Current state: " << cluster.state << std::endl;
                    break;
                }
            }

            // ===================================================================
            // Example 4: Cluster lifecycle management (optional)
            // ===================================================================
            if (!new_cluster_id.empty()) {
                std::cout << "\n4. Cluster lifecycle management (optional):" << std::endl;
                std::cout << "------------------------------------------" << std::endl;

                std::cout << "\nWould you like to demonstrate cluster lifecycle operations?" << std::endl;
                std::cout << "(This will wait for the cluster to start, then stop it)" << std::endl;
                std::cout << "Press 'y' to continue, or any other key to skip: ";

                char response;
                std::cin >> response;

                if (response == 'y' || response == 'Y') {
                    // Wait for cluster to be running
                    bool is_running = wait_for_cluster_state(compute, new_cluster_id, "RUNNING", 30, 20);

                    if (is_running) {
                        std::cout << "\n✓ Cluster is now running!" << std::endl;

                        // Demonstrate restart
                        std::cout << "\n4a. Restarting cluster..." << std::endl;
                        bool restarted = compute.restart_compute(new_cluster_id);
                        if (restarted) {
                            std::cout << "✓ Cluster restart initiated" << std::endl;
                        }

                        // Wait a bit then terminate
                        std::this_thread::sleep_for(std::chrono::seconds(10));

                        std::cout << "\n4b. Terminating cluster (to avoid ongoing costs)..." << std::endl;
                        bool terminated = compute.terminate_compute(new_cluster_id);
                        if (terminated) {
                            std::cout << "✓ Cluster termination initiated" << std::endl;
                        }
                    }
                } else {
                    std::cout << "\nSkipping lifecycle operations." << std::endl;
                    std::cout << "\nIMPORTANT: Don't forget to terminate the cluster manually" << std::endl;
                    std::cout << "to avoid ongoing costs! Cluster ID: " << new_cluster_id << std::endl;
                }
            }
        } else {
            std::cout << "\n✗ Cluster creation failed" << std::endl;
        }

        // ===================================================================
        // Example 5: Start a terminated cluster (if one exists)
        // ===================================================================
        std::cout << "\n5. Starting a terminated cluster (if available):" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;

        auto final_clusters = compute.list_compute();
        bool found_terminated = false;

        for (const auto& cluster : final_clusters) {
            if (cluster.state == "TERMINATED") {
                std::cout << "\nFound terminated cluster: " << cluster.cluster_id << std::endl;
                std::cout << "Name: " << cluster.cluster_name << std::endl;

                std::cout << "\nWould you like to start this cluster? (y/n): ";
                char response;
                std::cin >> response;

                if (response == 'y' || response == 'Y') {
                    std::cout << "Starting cluster..." << std::endl;
                    bool started = compute.start_compute(cluster.cluster_id);
                    if (started) {
                        std::cout << "✓ Cluster start initiated successfully!" << std::endl;
                        std::cout << "The cluster is now starting. Check Databricks UI for status." << std::endl;
                    }
                }

                found_terminated = true;
                break;
            }
        }

        if (!found_terminated) {
            std::cout << "No terminated clusters found." << std::endl;
        }

        // ===================================================================
        // Summary
        // ===================================================================
        std::cout << "\n========================================" << std::endl;
        std::cout << "Compute API example completed!" << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\nKey operations demonstrated:" << std::endl;
        std::cout << "  ✓ List all clusters" << std::endl;
        std::cout << "  ✓ Get cluster details" << std::endl;
        std::cout << "  ✓ Create a new cluster" << std::endl;
        std::cout << "  ✓ Start/restart/terminate clusters" << std::endl;

        std::cout << "\nIMPORTANT REMINDERS:" << std::endl;
        std::cout << "  - Running clusters incur costs" << std::endl;
        std::cout << "  - Terminate unused clusters to avoid charges" << std::endl;
        std::cout << "  - Check your Databricks workspace for any running clusters" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n========================================" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "========================================" << std::endl;
        std::cerr << "\nTroubleshooting:" << std::endl;
        std::cerr << "  1. Verify DATABRICKS_HOST and DATABRICKS_TOKEN are set" << std::endl;
        std::cerr << "  2. Check that your token has cluster management permissions" << std::endl;
        std::cerr << "  3. Ensure the node type (i3.xlarge) is available in your workspace" << std::endl;
        std::cerr << "  4. Verify the Spark version is supported" << std::endl;
        return 1;
    }

    return 0;
}