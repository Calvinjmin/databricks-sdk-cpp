// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
/**
 * @file jobs_example.cpp
 * @brief Example demonstrating the Databricks Jobs API
 *
 * This example shows how to:
 * 1. List all jobs in your workspace
 * 2. Get details for a specific job
 * 3. Trigger a job run
 */

#include "databricks/core/config.h"
#include "databricks/jobs/jobs.h"

#include <exception>
#include <iostream>

int main() {
    try {
        // Load configuration from environment
        databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

        std::cout << "Connecting to: " << auth.host << std::endl;
        std::cout << "======================================\n" << std::endl;

        // Create Jobs API client
        databricks::Jobs jobs(auth);

        // ===================================================================
        // Example 1: List all jobs
        // ===================================================================
        std::cout << "1. Listing all jobs:" << std::endl;
        std::cout << "-------------------" << std::endl;

        auto job_list = jobs.list_jobs(25, 0);
        std::cout << "Found " << job_list.size() << " jobs:\n" << std::endl;

        for (const auto& job : job_list) {
            std::cout << "  Job ID:      " << job.job_id << std::endl;
            std::cout << "  Name:        " << job.name << std::endl;
            std::cout << "  Creator:     " << job.creator_user_name << std::endl;
            std::cout << "  Created:     " << job.created_time << std::endl;
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 2: Get details for a specific job
        // ===================================================================
        if (!job_list.empty()) {
            uint64_t first_job_id = job_list[0].job_id;

            std::cout << "\n2. Getting details for job " << first_job_id << ":" << std::endl;
            std::cout << "-------------------------------------------" << std::endl;

            auto job_details = jobs.get_job(first_job_id);
            std::cout << "  Job ID:      " << job_details.job_id << std::endl;
            std::cout << "  Name:        " << job_details.name << std::endl;
            std::cout << "  Creator:     " << job_details.creator_user_name << std::endl;
            std::cout << "  Created:     " << job_details.created_time << std::endl;
            std::cout << std::endl;
        }

        std::cout << "\n======================================" << std::endl;
        std::cout << "Jobs API example completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
