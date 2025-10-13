#pragma once

#include "config.h"
#include <string>
#include <memory>
#include <vector>
#include <future>
#include <cstdint>

// Forward declare ODBC types to avoid including sql.h in header
typedef short SQLSMALLINT;

// ODBC SQL type constants (from sql.h)
#ifndef SQL_C_CHAR
#define SQL_C_CHAR 1
#endif

#ifndef SQL_VARCHAR
#define SQL_VARCHAR 12
#endif

namespace databricks
{
    /**
     * @brief Main client class for Databricks SQL operations
     *
     * This class provides a clean interface for executing SQL queries against Databricks.
     * It uses a modular configuration approach with AuthConfig, SQLConfig, and PoolingConfig.
     *
     * Example usage:
     * @code
     * // Simple usage with environment configuration
     * auto client = databricks::Client::Builder()
     *     .with_environment_config()
     *     .build();
     *
     * // Advanced usage with explicit configuration
     * databricks::AuthConfig auth;
     * auth.host = "https://your-workspace.cloud.databricks.com";
     * auth.token = "your_token";
     *
     * databricks::SQLConfig sql;
     * sql.http_path = "/sql/1.0/warehouses/abc123";
     *
     * auto client = databricks::Client::Builder()
     *     .with_auth(auth)
     *     .with_sql(sql)
     *     .with_pooling({.enabled = true, .max_connections = 20})
     *     .build();
     *
     * auto results = client.query("SELECT * FROM my_table");
     * @endcode
     */
    class Client
    {
    public:
        /**
         * @brief Parameter for parameterized queries
         */
        struct Parameter
        {
            std::string value;                  ///< Parameter value as string
            SQLSMALLINT c_type = SQL_C_CHAR;   ///< C data type (default: character)
            SQLSMALLINT sql_type = SQL_VARCHAR; ///< SQL data type (default: VARCHAR)
        };

        /**
         * @brief Builder pattern for constructing Client with modular configuration
         *
         * The Builder provides a fluent interface for configuring the Client.
         * It ensures that all required configuration is provided before building.
         */
        class Builder
        {
        public:
            Builder();

            /**
             * @brief Set authentication configuration
             * @param auth Authentication configuration
             * @return Builder reference for chaining
             */
            Builder& with_auth(const AuthConfig& auth);

            /**
             * @brief Set SQL configuration
             * @param sql SQL-specific configuration
             * @return Builder reference for chaining
             */
            Builder& with_sql(const SQLConfig& sql);

            /**
             * @brief Set connection pooling configuration (optional)
             * @param pooling Pooling configuration
             * @return Builder reference for chaining
             */
            Builder& with_pooling(const PoolingConfig& pooling);

            /**
             * @brief Load configuration from environment (profile + env vars)
             *
             * This is a convenience method that loads auth and SQL config from:
             * 1. ~/.databrickscfg profile (if exists)
             * 2. Environment variables (as fallback)
             *
             * @param profile Profile name to load (default: "DEFAULT")
             * @return Builder reference for chaining
             * @throws std::runtime_error if configuration cannot be loaded
             */
            Builder& with_environment_config(const std::string& profile = "DEFAULT");

            /**
             * @brief Enable auto-connect (connects immediately on build)
             * @param enable If true, connects immediately; if false, lazy connection (default: false)
             * @return Builder reference for chaining
             */
            Builder& with_auto_connect(bool enable = true);

            /**
             * @brief Set retry configuration for automatic error recovery
             *
             * Configures how the client handles transient failures like connection
             * timeouts and network errors. When enabled, failed operations will be
             * automatically retried with exponential backoff.
             *
             * @param retry Retry configuration
             * @return Builder reference for chaining
             *
             * @code
             * databricks::RetryConfig retry;
             * retry.max_attempts = 5;
             * retry.initial_backoff_ms = 200;
             *
             * auto client = databricks::Client::Builder()
             *     .with_environment_config()
             *     .with_retry(retry)
             *     .build();
             * @endcode
             */
            Builder& with_retry(const RetryConfig& retry);

            /**
             * @brief Build the Client
             *
             * @return Client instance
             * @throws std::runtime_error if required configuration is missing
             */
            Client build();

        private:
            std::unique_ptr<AuthConfig> auth_;
            std::unique_ptr<SQLConfig> sql_;
            std::unique_ptr<PoolingConfig> pooling_;
            std::unique_ptr<RetryConfig> retry_;
            bool auto_connect_ = false;
        };

        /**
         * @brief Destructor
         */
        ~Client();

        // Disable copy
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

        // Enable move
        Client(Client&&) noexcept;
        Client& operator=(Client&&) noexcept;

        /**
         * @brief Get the authentication configuration
         * @return const AuthConfig& Reference to auth configuration
         */
        const AuthConfig& get_auth_config() const;

        /**
         * @brief Get the SQL configuration
         * @return const SQLConfig& Reference to SQL configuration
         */
        const SQLConfig& get_sql_config() const;

        /**
         * @brief Get the pooling configuration
         * @return const PoolingConfig& Reference to pooling configuration
         */
        const PoolingConfig& get_pooling_config() const;

        /**
         * @brief Check if the client is configured with valid credentials
         * @return true if configured, false otherwise
         */
        bool is_configured() const;

        /**
         * @brief Execute a SQL query against Databricks
         *
         * This method supports both static queries and parameterized queries for security.
         *
         * **Static Query (no parameters):**
         * @code
         * auto results = client.query("SELECT * FROM my_table LIMIT 10");
         * @endcode
         *
         * **Parameterized Query (RECOMMENDED for dynamic values):**
         * Use placeholders (?) and provide parameters to prevent SQL injection:
         * @code
         * auto results = client.query(
         *     "SELECT * FROM users WHERE id = ? AND name = ?",
         *     {{"123"}, {"John"}}
         * );
         * @endcode
         *
         * @param sql The SQL query to execute (use ? for parameter placeholders)
         * @param params Optional vector of parameter values (default: empty = static query)
         * @return Results as a 2D vector of strings (rows and columns)
         *
         * @warning When using dynamic values (user input, variables), always use parameters
         *          to prevent SQL injection attacks. Never concatenate strings into SQL.
         */
        std::vector<std::vector<std::string>> query(
            const std::string& sql,
            const std::vector<Parameter>& params = {}
        );

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
         * @param params Optional vector of parameter values
         * @return std::future with query results
         */
        std::future<std::vector<std::vector<std::string>>> query_async(
            const std::string& sql,
            const std::vector<Parameter>& params = {}
        );

        /**
         * @brief Disconnect from Databricks
         */
        void disconnect();

    private:
        // Private constructor for Builder
        Client(const AuthConfig& auth, const SQLConfig& sql, const PoolingConfig& pooling, const RetryConfig& retry, bool auto_connect);

        class Impl;
        std::unique_ptr<Impl> pimpl_;

        friend class Builder;
    };

} // namespace databricks
