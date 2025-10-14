#pragma once

#include "config.h"

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <optional>

namespace databricks {

    /**
     * @brief Job Object for Databricks
     */
    struct Job {
        uint64_t job_id;
        std::string name;
        std::string creator_user_name;
        uint64_t created_time;  // Unix timestamp in milliseconds
        std::map<std::string, std::string> settings;

        static Job from_json(const std::string& json_str);
    };

    /**
     * @brief JobRun contains metadata regarding a ran Job
     */
    struct JobRun {
        uint64_t run_id;
        uint64_t job_id;
        std::string state;
        uint64_t start_time;  // Unix timestamp in milliseconds
        uint64_t end_time;    // Unix timestamp in milliseconds
        std::string result_state;

        static JobRun from_json(const std::string& json_str);
    };

    class Jobs {
        public:
            explicit Jobs( const AuthConfig& auth);
            ~Jobs();

            std::vector<Job> list_jobs( int limit = 25, int offset = 0 );
            Job get_job(uint64_t job_id);
            uint64_t run_now( uint64_t job_id, const std::map<std::string, std::string> notebooks_params = {} );

        private:
            class Impl;
            std::unique_ptr<Impl> pimpl_;

            static std::vector<Job> parse_jobs_list(const std::string& json);
            static std::vector<JobRun> parse_runs_list(const std::string& json);

    };
} // namespace databricks