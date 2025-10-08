#pragma once

#include <databricks/client.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

/**
 * @brief Helper functions for Databricks configuration examples
 */
namespace examples
{
    /**
     * @brief Load configuration from all available sources
     *
     * Uses the SDK's built-in configuration loading with the following precedence:
     * 1. Profile file (~/.databrickscfg) - if complete, uses exclusively
     * 2. Environment variables - only as fallback if profile missing/incomplete
     *
     * @param profile The profile name to load from ~/.databrickscfg (default: "DEFAULT")
     * @param verbose If true, print configuration source information (default: true)
     * @return Client::Config loaded from available sources
     * @throws std::runtime_error if no valid configuration is found
     */
    inline databricks::Client::Config load_config(const std::string& profile = "DEFAULT", bool verbose = true)
    {
        if (verbose) {
            // Check which source will be used before loading
            databricks::Client::Config test_config;
            bool profile_exists = test_config.load_profile_config(profile);
            
            if (profile_exists) {
                std::cout << "Configuration loaded from: Profile [" << profile << "]" << std::endl;
            } else {
                std::cout << "Configuration loaded from: Environment variables" << std::endl;
            }
        }

        // Use the SDK's factory method that handles all the logic
        return databricks::Client::Config::from_environment(profile);
    }

    /**
     * @brief Legacy function name for backward compatibility
     * @deprecated Use load_config() instead
     */
    inline databricks::Client::Config load_config_from_env()
    {
        return load_config("DEFAULT", true);
    }

    /**
     * @brief Print instructions for setting up environment variables
     */
    inline void print_env_setup_instructions()
    {
        std::cout << "To run this example, set the following environment variables:" << std::endl;
        std::cout << std::endl;
        std::cout << "export DATABRICKS_HOST=\"https://your-workspace.databricks.com\"" << std::endl;
        std::cout << "export DATABRICKS_TOKEN=\"your_databricks_token\"" << std::endl;
        std::cout << "export DATABRICKS_HTTP_PATH=\"/sql/1.0/warehouses/your_warehouse_id\"" << std::endl;
        std::cout << std::endl;
        std::cout << "Optional:" << std::endl;
        std::cout << "export DATABRICKS_TIMEOUT=120" << std::endl;
        std::cout << std::endl;
    }

} // namespace examples
