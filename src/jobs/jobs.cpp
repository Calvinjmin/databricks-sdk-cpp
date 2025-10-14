#include "databricks/jobs/jobs.h"
#include "../internal/http_client.h"
#include "../internal/logger.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace databricks {

    // Pimpl implementation class
    class Jobs::Impl {
    public:
        explicit Impl(const AuthConfig& auth)
            : http_client_(std::make_unique<internal::HttpClient>(auth)) {}

        std::unique_ptr<internal::HttpClient> http_client_;
    };

    // ============================================================================
    // Helper Functions
    // ============================================================================

    namespace {
        // Build query string from parameters
        std::string build_query_string(const std::map<std::string, std::string>& params) {
            if (params.empty()) {
                return "";
            }

            std::ostringstream oss;
            oss << "?";
            bool first = true;
            for (const auto& [key, value] : params) {
                if (!first) {
                    oss << "&";
                }
                oss << key << "=" << value;
                first = false;
            }
            return oss.str();
        }
    }

    // ============================================================================
    // Job and JobRun JSON Parsing
    // ============================================================================

    Job Job::from_json(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            Job job;

            // Use auto to preserve the original JSON types
            job.job_id = j.value("job_id", uint64_t(0));
            job.name = j.value("name", "");
            job.creator_user_name = j.value("creator_user_name", "");
            job.created_time = j.value("created_time", uint64_t(0));

            // Extract settings object as a raw JSON string if present
            if (j.contains("settings")) {
                job.settings["raw"] = j["settings"].dump();
            }

            return job;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Job JSON: " + std::string(e.what()));
        }
    }

    JobRun JobRun::from_json(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            JobRun run;

            // Use auto to preserve the original JSON types
            run.run_id = j.value("run_id", uint64_t(0));
            run.job_id = j.value("job_id", uint64_t(0));
            run.start_time = j.value("start_time", uint64_t(0));
            run.end_time = j.value("end_time", uint64_t(0));

            // Extract state information
            if (j.contains("state")) {
                run.state = j["state"].value("life_cycle_state", "");
                run.result_state = j["state"].value("result_state", "");
            }

            return run;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse JobRun JSON: " + std::string(e.what()));
        }
    }

    // ============================================================================
    // Jobs Constructor and Destructor
    // ============================================================================

    Jobs::Jobs(const AuthConfig& auth)
        : pimpl_(std::make_unique<Impl>(auth)) {}

    Jobs::~Jobs() = default;

    // ============================================================================
    // Public API Methods
    // ============================================================================

    std::vector<Job> Jobs::list_jobs(int limit, int offset) {
        internal::get_logger()->info("Listing jobs (limit=" + std::to_string(limit) +
                                     ", offset=" + std::to_string(offset) + ")");

        // Build query parameters
        std::map<std::string, std::string> params;
        if (limit > 0) {
            params["limit"] = std::to_string(limit);
        }
        if (offset > 0) {
            params["offset"] = std::to_string(offset);
        }
        params["expand_tasks"] = "false";

        // Make API request
        std::string query = build_query_string(params);
        auto response = pimpl_->http_client_->get("/jobs/list" + query);

        if (response.status_code != 200) {
            std::string error_msg = "Failed to list jobs: HTTP " +
                                   std::to_string(response.status_code) +
                                   " - " + response.body;
            internal::get_logger()->error(error_msg);
            throw std::runtime_error(error_msg);
        }

        internal::get_logger()->debug("Jobs list response: " + response.body);
        return parse_jobs_list(response.body);
    }

    Job Jobs::get_job(uint64_t job_id) {
        internal::get_logger()->info("Getting job details for job_id=" + std::to_string(job_id));

        // Build query parameters
        std::map<std::string, std::string> params;
        params["job_id"] = std::to_string(job_id);

        // Make API request
        std::string query = build_query_string(params);
        auto response = pimpl_->http_client_->get("/jobs/get" + query);

        if (response.status_code != 200) {
            std::string error_msg = "Failed to get job: HTTP " +
                                   std::to_string(response.status_code) +
                                   " - " + response.body;
            internal::get_logger()->error(error_msg);
            throw std::runtime_error(error_msg);
        }

        internal::get_logger()->debug("Job details response: " + response.body);
        return Job::from_json(response.body);
    }

    uint64_t Jobs::run_now(uint64_t job_id, const std::map<std::string, std::string>& notebook_params) {
        internal::get_logger()->info("Running job_id=" + std::to_string(job_id));

        // Build request body using nlohmann/json
        json body_json;
        body_json["job_id"] = job_id;

        if (!notebook_params.empty()) {
            body_json["notebook_params"] = notebook_params;
        }

        std::string body = body_json.dump();
        internal::get_logger()->debug("Run now request body: " + body);

        // Make API request
        auto response = pimpl_->http_client_->post("/jobs/run-now", body);

        if (response.status_code != 200) {
            std::string error_msg = "Failed to run job: HTTP " +
                                   std::to_string(response.status_code) +
                                   " - " + response.body;
            internal::get_logger()->error(error_msg);
            throw std::runtime_error(error_msg);
        }

        internal::get_logger()->debug("Run now response: " + response.body);

        // Extract run_id from response
        try {
            auto response_json = json::parse(response.body);
            uint64_t run_id = response_json.value("run_id", uint64_t(0));

            if (run_id == 0) {
                throw std::runtime_error("run_id not found or is 0 in response");
            }

            internal::get_logger()->info("Job started with run_id=" + std::to_string(run_id));
            return run_id;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse run response: " + std::string(e.what()));
        }
    }

    // ============================================================================
    // Private Helper Methods
    // ============================================================================

    std::vector<Job> Jobs::parse_jobs_list(const std::string& json_str) {
        std::vector<Job> jobs;

        try {
            auto j = json::parse(json_str);

            if (!j.contains("jobs") || !j["jobs"].is_array()) {
                internal::get_logger()->warn("No jobs array found in response");
                return jobs;
            }

            for (const auto& job_json : j["jobs"]) {
                jobs.push_back(Job::from_json(job_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(jobs.size()) + " jobs");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse jobs list: " + std::string(e.what()));
            throw std::runtime_error("Failed to parse jobs list: " + std::string(e.what()));
        }

        return jobs;
    }

    std::vector<JobRun> Jobs::parse_runs_list(const std::string& json_str) {
        std::vector<JobRun> runs;

        try {
            auto j = json::parse(json_str);

            if (!j.contains("runs") || !j["runs"].is_array()) {
                internal::get_logger()->warn("No runs array found in response");
                return runs;
            }

            for (const auto& run_json : j["runs"]) {
                runs.push_back(JobRun::from_json(run_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(runs.size()) + " runs");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse runs list: " + std::string(e.what()));
            throw std::runtime_error("Failed to parse runs list: " + std::string(e.what()));
        }

        return runs;
    }

} // namespace databricks
