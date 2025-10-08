#pragma once

#include <string>
#include <memory>
#include <vector>
#include <future>

namespace databricks
{

    /**
     * @brief Main client class for interacting with Databricks
     *
     * This class provides the primary interface for all Databricks SDK operations.
     * Supports both synchronous and asynchronous connection modes.
     */
    class Client
    {
    public:
        /**
         * @brief Configuration options for the Client
         */
        struct Config
        {
            std::string host;         ///< Databricks workspace URL
            std::string token;        ///< Authentication token
            std::string http_path;    ///< HTTP path for SQL warehouse/cluster
            int timeout_seconds = 60; ///< Request timeout in seconds

            // Connection pooling settings (optional performance optimization)
            bool enable_pooling = false;        ///< Enable connection pooling (default: false)
            size_t pool_min_connections = 1;    ///< Minimum connections in pool (default: 1)
            size_t pool_max_connections = 10;   ///< Maximum connections in pool (default: 10)

            /**
             * @brief Generate a hash for this config (used for pool sharing)
             */
            size_t hash() const;

            /**
             * @brief Check if two configs are equivalent for pooling purposes
             */
            bool equivalent_for_pooling(const Config& other) const;

            /**
             * @brief Load Databricks profile configuration from the default CLI config file.
             * 
             * Attempts to populate the current Config object (`host`, `token`, `http_path`) 
             * from the Databricks CLI configuration file (`~/.databrickscfg`).  
             * 
             * @param profile The name of the profile to load (default: `"DEFAULT"`).
             *                Must match a section in the config file like `[DEFAULT]`.
             * 
             * @return true if the profile was found and all required fields were loaded; 
             *         false otherwise.
             *
             * @note This function modifies the current Config object in-place.
             */
            bool load_profile_config(const std::string& profile = "DEFAULT");

            /**
             * @brief Load configuration from environment variables.
             * 
             * Attempts to populate the current Config object from environment variables:
             * - DATABRICKS_HOST or DATABRICKS_SERVER_HOSTNAME
             * - DATABRICKS_TOKEN or DATABRICKS_ACCESS_TOKEN
             * - DATABRICKS_HTTP_PATH or DATABRICKS_SQL_HTTP_PATH
             * - DATABRICKS_TIMEOUT (optional)
             * 
             * @return true if all required environment variables were found; false otherwise.
             *
             * @note This function modifies the current Config object in-place, only 
             *       overriding fields that have corresponding environment variables set.
             */
            bool load_from_env();

            /**
             * @brief Create a Config by loading from all available sources.
             * 
             * Loads configuration with the following precedence (highest to lowest):
             * 1. Profile configuration file (~/.databrickscfg) - if complete, stops here
             * 2. Environment variables - only used as fallback if profile is missing/incomplete
             * 
             * This gives profile configuration priority, with environment variables as a
             * fallback for when no profile is configured. The profile is "all or nothing" -
             * if it exists and has all required fields, it's used exclusively.
             * 
             * @param profile The profile name to load from ~/.databrickscfg (default: "DEFAULT")
             * @return Config object populated from available sources
             * @throws std::runtime_error if no valid configuration is found
             */
            static Config from_environment(const std::string& profile = "DEFAULT");
        };

        /**
         * @brief Construct a new Client with default configuration
         *
         * Note: Connection is NOT established until first query or explicit connect() call.
         */
        Client();

        /**
         * @brief Construct a new Client with custom configuration
         * @param config Configuration options
         * @param auto_connect If true, connects immediately; if false, uses lazy connection (default: false)
         */
        explicit Client(const Config &config, bool auto_connect = false);

        /**
         * @brief Destructor
         */
        ~Client();

        // Disable copy
        Client(const Client &) = delete;
        Client &operator=(const Client &) = delete;

        // Enable move
        Client(Client &&) noexcept;
        Client &operator=(Client &&) noexcept;

        /**
         * @brief Get the current configuration
         * @return const Config& Reference to the configuration
         */
        const Config &get_config() const;

        /**
         * @brief Check if the client is configured with valid credentials
         * @return true if configured, false otherwise
         */
        bool is_configured() const;

        /**
         * @brief Execute a SQL query against Databricks
         * @param sql The SQL query to execute
         * @return Results as a 2D vector of strings (rows and columns)
         */
        std::vector<std::vector<std::string>> query(const std::string& sql);

        /**
         * @brief Explicitly establish connection to Databricks
         *
         * Normally not needed as query() will auto-connect. Useful for
         * validating credentials or pre-warming connection.
         *
         * @throws std::runtime_error if connection fails
         */
        void connect();

        /**
         * @brief Asynchronously establish connection to Databricks
         *
         * Non-blocking connection useful for reducing perceived latency.
         * Call wait() on returned future before executing queries, or
         * query() will automatically wait.
         *
         * @return std::future<void> Future that completes when connected
         */
        std::future<void> connect_async();

        /**
         * @brief Asynchronously execute a SQL query
         *
         * @param sql The SQL query to execute
         * @return std::future with query results
         */
        std::future<std::vector<std::vector<std::string>>> query_async(const std::string& sql);

        /**
         * @brief Disconnect from Databricks
         */
        void disconnect();

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;
    };

} // namespace databricks
