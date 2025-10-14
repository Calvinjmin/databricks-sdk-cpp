#pragma once

#include <databricks/core/client.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

/**
 * @brief Helper functions for Databricks configuration examples
 */
namespace examples
{
    /**
     * @brief Create and configure a Client::Builder with environment configuration
     *
     * Uses the SDK's built-in configuration loading with the following precedence:
     * 1. Profile file (~/.databrickscfg) - if complete, uses exclusively
     * 2. Environment variables - only as fallback if profile missing/incomplete
     *
     * This helper is designed to be used directly in a fluent call chain:
     * auto client = examples::get_builder().build();
     * or with additional configuration:
     * auto client = examples::get_builder().with_pooling(pooling_config).build();
     *
     * @param profile The profile name to load from ~/.databrickscfg (default: "DEFAULT")
     * @param verbose If true, print configuration source information (default: true)
     * @return Client configured with environment settings
     * @throws std::runtime_error if no valid configuration is found
     */
    inline databricks::Client build_client(const std::string& profile = "DEFAULT", bool verbose = true)
    {
        if (verbose) {
            std::cout << "Loading configuration from environment/profile [" << profile << "]..." << std::endl;
        }

        // Use the SDK's Builder with environment configuration
        return databricks::Client::Builder()
            .with_environment_config(profile)
            .build();
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
        std::cout << "Or create a ~/.databrickscfg file with a [DEFAULT] section." << std::endl;
    }

} // namespace examples
