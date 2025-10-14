#include "databricks/core/config.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

namespace databricks
{
    // ========== AuthConfig Implementation ==========

    AuthConfig AuthConfig::from_profile(const std::string& profile)
    {
        const char* home = std::getenv("HOME");
        if (!home) {
            throw std::runtime_error("HOME environment variable not set");
        }

        std::ifstream file(std::string(home) + "/.databrickscfg");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open ~/.databrickscfg");
        }

        AuthConfig config;
        std::string line;
        bool in_profile = false;
        bool found_host = false, found_token = false;

        while (std::getline(file, line)) {
            // Trim whitespace
            auto start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            auto end = line.find_last_not_of(" \t");
            line = line.substr(start, end - start + 1);

            if (line.empty() || line[0] == '#') continue;

            if (line.front() == '[' && line.back() == ']') {
                in_profile = (line.substr(1, line.size() - 2) == profile);
                continue;
            }

            if (!in_profile) continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim key/value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "host") {
                config.host = value;
                found_host = true;
            } else if (key == "token") {
                config.token = value;
                found_token = true;
            }
        }

        if (!found_host || !found_token) {
            throw std::runtime_error("Profile [" + profile + "] missing required fields (host, token)");
        }

        return config;
    }

    AuthConfig AuthConfig::from_env()
    {
        AuthConfig config;

        // Load host
        const char* host_env = std::getenv("DATABRICKS_HOST");
        if (!host_env) host_env = std::getenv("DATABRICKS_SERVER_HOSTNAME");
        if (!host_env) {
            throw std::runtime_error("DATABRICKS_HOST or DATABRICKS_SERVER_HOSTNAME environment variable not set");
        }
        config.host = host_env;

        // Load token
        const char* token_env = std::getenv("DATABRICKS_TOKEN");
        if (!token_env) token_env = std::getenv("DATABRICKS_ACCESS_TOKEN");
        if (!token_env) {
            throw std::runtime_error("DATABRICKS_TOKEN or DATABRICKS_ACCESS_TOKEN environment variable not set");
        }
        config.token = token_env;

        // Load optional timeout
        const char* timeout_env = std::getenv("DATABRICKS_TIMEOUT");
        if (timeout_env) {
            config.timeout_seconds = std::atoi(timeout_env);
        }

        return config;
    }

    AuthConfig AuthConfig::from_environment(const std::string& profile)
    {
        // Try profile first
        try {
            return from_profile(profile);
        } catch (...) {
            // Profile failed, try environment variables
        }

        // Try environment variables
        try {
            return from_env();
        } catch (...) {
            // Both failed
        }

        throw std::runtime_error(
            "Failed to load Databricks authentication configuration. Ensure either:\n"
            "  1. ~/.databrickscfg has a [" + profile + "] section with host and token, OR\n"
            "  2. Environment variables are set: DATABRICKS_HOST and DATABRICKS_TOKEN"
        );
    }

    bool AuthConfig::is_valid() const
    {
        return !host.empty() && !token.empty() && timeout_seconds > 0;
    }

    // ========== SQLConfig Implementation ==========

    bool SQLConfig::is_valid() const
    {
        return !http_path.empty() && !odbc_driver_name.empty();
    }

    // ========== PoolingConfig Implementation ==========

    bool PoolingConfig::is_valid() const
    {
        return min_connections > 0 &&
               max_connections >= min_connections &&
               connection_timeout_ms > 0;
    }

    // ========== RetryConfig Implementation ==========

    bool RetryConfig::is_valid() const
    {
        return max_attempts > 0 &&
               initial_backoff_ms > 0 &&
               backoff_multiplier > 0.0 &&
               max_backoff_ms >= initial_backoff_ms;
    }

} // namespace databricks
