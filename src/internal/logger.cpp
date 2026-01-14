// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "logger.h"

#include "path_utils.h"

#include <cstdlib>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace databricks {
namespace internal {
std::shared_ptr<spdlog::logger> get_logger() {
    static std::shared_ptr<spdlog::logger> logger = []() {
        std::shared_ptr<spdlog::logger> log;

        // Check for log file environment variable
        const char* log_file_env = std::getenv("DATABRICKS_LOG_FILE");

        try {
            if (log_file_env && std::strlen(log_file_env) > 0) {
                // Validate the log file path to prevent path traversal attacks
                std::string log_file;
                try {
                    log_file = validate_env_path(log_file_env, "DATABRICKS_LOG_FILE");
                } catch (const std::invalid_argument& e) {
                    // If validation fails, log warning to stderr and use stderr sink
                    auto temp_log = spdlog::stderr_color_mt("databricks_temp");
                    temp_log->warn("Invalid DATABRICKS_LOG_FILE path: {}. Using stderr instead.", e.what());
                    spdlog::drop("databricks_temp");
                    log = spdlog::stderr_color_mt("databricks");
                    return log;
                }

                // Log to file
                log = spdlog::basic_logger_mt("databricks", log_file);
            } else {
                // Log to stderr with colors
                log = spdlog::stderr_color_mt("databricks");
            }
        } catch (const spdlog::spdlog_ex& ex) {
            // Fallback to stderr if file creation fails
            log = spdlog::stderr_color_mt("databricks");
        }

        // Set log level from environment (default: INFO)
        const char* log_level_str = std::getenv("DATABRICKS_LOG_LEVEL");
        spdlog::level::level_enum level = spdlog::level::info; // Default

        if (log_level_str) {
            std::string level_upper(log_level_str);
            // Convert to uppercase
            for (char& c : level_upper) {
                c = std::toupper(c);
            }

            if (level_upper == "DEBUG" || level_upper == "TRACE") {
                level = spdlog::level::debug;
            } else if (level_upper == "INFO") {
                level = spdlog::level::info;
            } else if (level_upper == "WARN" || level_upper == "WARNING") {
                level = spdlog::level::warn;
            } else if (level_upper == "ERROR" || level_upper == "ERR") {
                level = spdlog::level::err;
            } else if (level_upper == "OFF" || level_upper == "NONE") {
                level = spdlog::level::off;
            }
        }

        log->set_level(level);

        // Set pattern: [timestamp] [level] [logger_name] message
        log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        return log;
    }();

    return logger;
};
} // namespace internal
} // namespace databricks