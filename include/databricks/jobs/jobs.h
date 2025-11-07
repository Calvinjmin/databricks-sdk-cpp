// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include "databricks/core/config.h"
#include "databricks/jobs/jobs_types.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace databricks {
// Forward declaration for dependency injection
namespace internal {
class IHttpClient;
}

/**
 * @brief Client for interacting with the Databricks Jobs API
 *
 * The Jobs API allows you to create, manage, and trigger jobs in your Databricks workspace.
 * This implementation uses Jobs API 2.2 for full feature support including pagination.
 *
 * Example usage:
 * @code
 * databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
 * databricks::Jobs jobs(auth);
 *
 * // List all jobs
 * auto job_list = jobs.list_jobs(25, 0);
 *
 * // Get specific job details
 * auto job = jobs.get_job(123456789);
 *
 * // Trigger a job run
 * std::map<std::string, std::string> params = {{"key", "value"}};
 * uint64_t run_id = jobs.run_now(123456789, params);
 * @endcode
 */
class Jobs {
public:
    /**
     * @brief Construct a Jobs API client
     * @param auth Authentication configuration with host and token
     */
    explicit Jobs(const AuthConfig& auth);

    /**
     * @brief Construct a Jobs API client with dependency injection (for testing)
     * @param http_client Injected HTTP client (use MockHttpClient for unit tests)
     * @note This constructor is primarily for testing with mock HTTP clients
     */
    explicit Jobs(std::shared_ptr<internal::IHttpClient> http_client);

    /**
     * @brief Destructor
     */
    ~Jobs();

    // Disable copy
    Jobs(const Jobs&) = delete;
    Jobs& operator=(const Jobs&) = delete;

    /**
     * @brief List jobs in the workspace with pagination
     *
     * @param limit Maximum number of jobs to return (default: 25, max: 100)
     * @param offset Number of jobs to skip for pagination (default: 0)
     * @return Vector of Job objects
     * @throws std::runtime_error if the API request fails
     *
     * @note Uses Jobs API 2.2 for pagination support
     */
    std::vector<Job> list_jobs(int limit = 25, int offset = 0);

    /**
     * @brief Get detailed information about a specific job
     *
     * @param job_id The unique identifier of the job
     * @return Job object with full details
     * @throws std::runtime_error if the job is not found or the API request fails
     */
    Job get_job(uint64_t job_id);

    /**
     * @brief Trigger an immediate run of a job
     *
     * @param job_id The unique identifier of the job to run
     * @param notebook_params Optional parameters to pass to the notebook (for notebook jobs)
     * @return The run_id of the triggered job run
     * @throws std::runtime_error if the API request fails
     *
     * @note This method returns immediately after triggering the job. Use the run_id
     *       to poll for status or wait for completion.
     */
    uint64_t run_now(uint64_t job_id, const std::map<std::string, std::string>& notebook_params = {});

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;

    static std::vector<Job> parse_jobs_list(const std::string& json);
    static std::vector<JobRun> parse_runs_list(const std::string& json);
};

} // namespace databricks