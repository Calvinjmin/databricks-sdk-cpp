// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace databricks {

/**
 * @brief Represents a Databricks job
 *
 * Jobs are the primary method for running data processing workloads in Databricks.
 * This struct contains the core metadata about a job configuration.
 */
struct Job {
    uint64_t job_id = 0;                         ///< Unique identifier for the job
    std::string name;                            ///< Display name of the job
    std::string creator_user_name;               ///< Username of the job creator
    uint64_t created_time = 0;                   ///< Unix timestamp in milliseconds when the job was created
    std::map<std::string, std::string> settings; ///< Job configuration settings (raw JSON)

    /**
     * @brief Parse a Job from JSON string
     * @param json_str JSON representation of a job
     * @return Parsed Job object
     * @throws std::runtime_error if parsing fails
     */
    static Job from_json(const std::string& json_str);
};

/**
 * @brief Represents a specific execution (run) of a Databricks job
 *
 * Each time a job is triggered, it creates a new run with its own unique run_id.
 * This struct tracks the state and timing information for a job execution.
 */
struct JobRun {
    uint64_t run_id = 0;      ///< Unique identifier for this job run
    uint64_t job_id = 0;      ///< ID of the job that was executed
    std::string state;        ///< Lifecycle state (e.g., "RUNNING", "TERMINATED")
    uint64_t start_time = 0;  ///< Unix timestamp in milliseconds when the run started
    uint64_t end_time = 0;    ///< Unix timestamp in milliseconds when the run ended (0 if still running)
    std::string result_state; ///< Result state (e.g., "SUCCESS", "FAILED")

    /**
     * @brief Parse a JobRun from JSON string
     * @param json_str JSON representation of a job run
     * @return Parsed JobRun object
     * @throws std::runtime_error if parsing fails
     */
    static JobRun from_json(const std::string& json_str);
};

} // namespace databricks
