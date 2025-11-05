#pragma once

#include <string>
#include <cstddef>
#include "databricks/internal/secure_string.h"

namespace databricks
{
    /**
     * @brief Core authentication configuration shared across all Databricks features
     *
     * This configuration contains the fundamental authentication and connection
     * settings needed to connect to Databricks. It can be shared across multiple
     * client types (SQL, Workspace, Delta, etc.).
     */
    struct AuthConfig
    {
        std::string host;         ///< Databricks workspace URL (e.g., "https://your-workspace.cloud.databricks.com")
        std::string token;        ///< Authentication token (personal access token or OAuth token) - Note: For internal compatibility only. Token is stored securely internally.
        int timeout_seconds = 60; ///< Request timeout in seconds (default: 60)

    private:
        internal::SecureString secure_token_; ///< Secure storage for authentication token

    public:
        /**
         * @brief Set the authentication token securely
         *
         * Stores the token in secure memory that is locked and zeroed on destruction.
         * Also updates the public token field for backward compatibility.
         *
         * @param t The authentication token to set
         */
        void set_token(const std::string& t) {
            token = t;
            secure_token_ = internal::to_secure_string(t);
        }

        /**
         * @brief Get the securely stored token
         *
         * Returns a reference to the secure token storage.
         *
         * @return const reference to SecureString containing the token
         */
        const internal::SecureString& get_secure_token() const {
            return secure_token_;
        }

        /**
         * @brief Check if a secure token has been set
         *
         * @return true if a secure token is available, false otherwise
         */
        bool has_secure_token() const {
            return !secure_token_.empty();
        }

        /**
         * @brief Load authentication configuration from Databricks CLI profile
         *
         * Reads from ~/.databrickscfg file and loads the specified profile section.
         *
         * @param profile The profile name to load (default: "DEFAULT")
         * @return AuthConfig populated from the profile
         * @throws std::runtime_error if profile is not found or incomplete
         */
        static AuthConfig from_profile(const std::string& profile = "DEFAULT");

        /**
         * @brief Load authentication configuration from environment variables
         *
         * Reads from:
         * - DATABRICKS_HOST or DATABRICKS_SERVER_HOSTNAME
         * - DATABRICKS_TOKEN or DATABRICKS_ACCESS_TOKEN
         * - DATABRICKS_TIMEOUT (optional)
         *
         * @return AuthConfig populated from environment variables
         * @throws std::runtime_error if required environment variables are not set
         */
        static AuthConfig from_env();

        /**
         * @brief Load authentication configuration from all available sources
         *
         * Precedence (highest to lowest):
         * 1. Profile configuration file (~/.databrickscfg) - if complete, stops here
         * 2. Environment variables - used as fallback if profile missing/incomplete
         *
         * @param profile The profile name to try loading (default: "DEFAULT")
         * @return AuthConfig populated from available sources
         * @throws std::runtime_error if no valid configuration is found
         */
        static AuthConfig from_environment(const std::string& profile = "DEFAULT");

        /**
         * @brief Validate that all required fields are set
         * @return true if valid, false otherwise
         */
        bool is_valid() const;
    };

    /**
     * @brief SQL-specific configuration for Databricks SQL operations
     *
     * Contains settings specific to SQL query execution, including the
     * HTTP path to the SQL warehouse/cluster and ODBC driver configuration.
     */
    struct SQLConfig
    {
        std::string http_path;    ///< HTTP path for SQL warehouse/cluster (e.g., "/sql/1.0/warehouses/abc123")
        std::string odbc_driver_name = "Simba Spark ODBC Driver"; ///< ODBC driver name (default: Simba Spark ODBC Driver)

        /**
         * @brief Validate that all required fields are set
         * @return true if valid, false otherwise
         */
        bool is_valid() const;
    };

    /**
     * @brief Connection pooling configuration (optional performance optimization)
     *
     * Enables connection pooling to improve performance for applications
     * that execute many queries. Pooling reduces connection overhead by
     * reusing existing connections.
     */
    struct PoolingConfig
    {
        bool enabled = false;             ///< Enable connection pooling (default: false)
        size_t min_connections = 1;       ///< Minimum connections to maintain in pool (default: 1)
        size_t max_connections = 10;      ///< Maximum connections allowed in pool (default: 10)
        int connection_timeout_ms = 5000; ///< Timeout for acquiring connection from pool in milliseconds (default: 5000)

        /**
         * @brief Validate configuration values
         * @return true if valid, false otherwise
         */
        bool is_valid() const;
    };

    /**
     * @brief Retry configuration for automatic error recovery
     *
     * Configures automatic retry behavior for transient failures such as
     * connection timeouts, network errors, and rate limits. Uses exponential
     * backoff to avoid overwhelming the server during retries.
     *
     * Example usage:
     * @code
     * databricks::RetryConfig retry;
     * retry.enabled = true;
     * retry.max_attempts = 5;
     * retry.initial_backoff_ms = 200;
     *
     * auto client = databricks::Client::Builder()
     *     .with_environment_config()
     *     .with_retry(retry)
     *     .build();
     * @endcode
     */
    struct RetryConfig
    {
        bool enabled = true;                    ///< Enable automatic retries (default: true)
        size_t max_attempts = 3;                ///< Maximum retry attempts (default: 3)
        size_t initial_backoff_ms = 100;        ///< Initial backoff in milliseconds (default: 100ms)
        double backoff_multiplier = 2.0;        ///< Exponential backoff multiplier (default: 2x)
        size_t max_backoff_ms = 10000;          ///< Maximum backoff cap (default: 10s)
        bool retry_on_timeout = true;           ///< Retry on connection timeout (default: true)
        bool retry_on_connection_lost = true;   ///< Retry on connection errors (default: true)

        /**
         * @brief Validate configuration values
         * @return true if valid, false otherwise
         */
        bool is_valid() const;
    };

} // namespace databricks
