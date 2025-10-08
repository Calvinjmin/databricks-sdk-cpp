#pragma once

#include <databricks/client.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

/**
 * @brief Helper to load Databricks configuration from environment variables
 */
namespace examples
{
    /**
     * @brief Load configuration from environment variables
     *
     * Reads the following environment variables:
     * - DATABRICKS_HOST or DATABRICKS_SERVER_HOSTNAME
     * - DATABRICKS_TOKEN or DATABRICKS_ACCESS_TOKEN
     * - DATABRICKS_HTTP_PATH
     * - DATABRICKS_TIMEOUT (optional, defaults to 120)
     *
     * @return Client::Config loaded from environment
     * @throws std::runtime_error if required variables are missing
     */
    inline databricks::Client::Config load_config_from_env()
    {
        databricks::Client::Config config;

        // Try to get host
        const char *host = std::getenv("DATABRICKS_HOST");
        if (!host)
        {
            host = std::getenv("DATABRICKS_SERVER_HOSTNAME");
        }
        if (!host)
        {
            throw std::runtime_error(
                "Missing DATABRICKS_HOST or DATABRICKS_SERVER_HOSTNAME environment variable.\n"
                "Set it with: export DATABRICKS_HOST=\"https://your-workspace.databricks.com\"");
        }
        config.host = host;

        // Try to get token
        const char *token = std::getenv("DATABRICKS_TOKEN");
        if (!token)
        {
            token = std::getenv("DATABRICKS_ACCESS_TOKEN");
        }
        if (!token)
        {
            throw std::runtime_error(
                "Missing DATABRICKS_TOKEN or DATABRICKS_ACCESS_TOKEN environment variable.\n"
                "Set it with: export DATABRICKS_TOKEN=\"your_token\"");
        }
        config.token = token;

        // Get HTTP path
        const char *http_path = std::getenv("DATABRICKS_HTTP_PATH");
        if (!http_path)
        {
            throw std::runtime_error(
                "Missing DATABRICKS_HTTP_PATH environment variable.\n"
                "Set it with: export DATABRICKS_HTTP_PATH=\"/sql/1.0/warehouses/your_warehouse_id\"");
        }
        config.http_path = http_path;

        // Get timeout (optional, defaults to 120)
        const char *timeout = std::getenv("DATABRICKS_TIMEOUT");
        if (timeout)
        {
            config.timeout_seconds = std::atoi(timeout);
        }
        else
        {
            config.timeout_seconds = 120; // Default
        }

        return config;
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
