// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "databricks/core/client.h"

#include "databricks/connection_pool.h"

#include "../internal/logger.h"
#include "../internal/pool_manager.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <sql.h>
#include <sqlext.h>

namespace databricks {
// ========== Client::Impl ==========

/**
 * @brief Private implementation class (PIMPL pattern)
 */
class Client::Impl {
public:
    AuthConfig auth;
    SQLConfig sql;
    PoolingConfig pooling;
    RetryConfig retry;
    SQLHENV henv; // Environment handle
    SQLHDBC hdbc; // Connection handle
    bool connected;
    std::mutex connection_mutex;            // Thread safety for connection operations
    std::future<void> async_connect_future; // Track async connection
    std::shared_ptr<ConnectionPool> pool;   // Shared pool (if pooling enabled)

    explicit Impl(const AuthConfig& auth_cfg, const SQLConfig& sql_cfg, const PoolingConfig& pool_cfg,
                  const RetryConfig& retry_cfg, bool auto_connect)
        : auth(auth_cfg)
        , sql(sql_cfg)
        , pooling(pool_cfg)
        , retry(retry_cfg)
        , henv(SQL_NULL_HENV)
        , hdbc(SQL_NULL_HDBC)
        , connected(false)
        , pool(nullptr) {
        internal::get_logger()->debug("Initializing Databricks client");

        // Validate configurations
        if (!auth.is_valid()) {
            internal::get_logger()->error("Invalid AuthConfig: missing required fields");
            throw std::runtime_error("Invalid AuthConfig: host, token, and timeout_seconds are required");
        }
        if (!sql.is_valid()) {
            internal::get_logger()->error("Invalid SQLConfig: missing required fields");
            throw std::runtime_error("Invalid SQLConfig: http_path and odbc_driver_name are required");
        }

        // If pooling is enabled, get/create shared pool and return early
        if (pooling.enabled) {
            internal::get_logger()->info("Connection pooling enabled (min: {}, max: {})", pooling.min_connections,
                                         pooling.max_connections);
            pool = internal::PoolManager::instance().get_pool(auth, sql, pooling);

            if (auto_connect) {
                internal::get_logger()->debug("Starting async pool warm-up");
                pool->warm_up_async();
            }
            return; // Don't allocate ODBC handles for pooled clients
        }

        // Non-pooled client: allocate dedicated ODBC connection
        internal::get_logger()->debug("Allocating dedicated ODBC connection (non-pooled)");
        // Allocate environment handle
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
        if (!SQL_SUCCEEDED(ret)) {
            internal::get_logger()->error("Failed to allocate ODBC environment handle");
            throw std::runtime_error("Failed to allocate ODBC environment handle");
        }

        // Set ODBC version
        ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (!SQL_SUCCEEDED(ret)) {
            internal::get_logger()->error("Failed to set ODBC version");
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            throw std::runtime_error("Failed to set ODBC version");
        }

        // Allocate connection handle
        ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
        if (!SQL_SUCCEEDED(ret)) {
            internal::get_logger()->error("Failed to allocate ODBC connection handle");
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
            throw std::runtime_error("Failed to allocate ODBC connection handle");
        }

        // Set connection timeouts
        SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
        SQLSetConnectAttr(hdbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)30, 0);

        if (auto_connect) {
            connect();
        }
    }

    ~Impl() {
        // Pooled clients don't own ODBC handles
        if (pool) {
            return;
        }

        // Non-pooled clients: clean up owned resources
        disconnect();
        if (hdbc != SQL_NULL_HDBC) {
            SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        }
        if (henv != SQL_NULL_HENV) {
            SQLFreeHandle(SQL_HANDLE_ENV, henv);
        }
    }

    std::string build_connection_string() {
        // Strip https:// or http:// from host if present
        std::string host = auth.host;
        if (host.find("https://") == 0) {
            host = host.substr(8);
        } else if (host.find("http://") == 0) {
            host = host.substr(7);
        }

        // Use secure token if available, fallback to regular token for backward compatibility
        std::string token_str;
        token_str = internal::from_secure_string(auth.get_secure_token());

        // Build connection string
        std::ostringstream connStr;
        connStr << "Driver=" << sql.odbc_driver_name << ";"
                << "Host=" << host << ";"
                << "Port=443;"
                << "HTTPPath=" << sql.http_path << ";"
                << "AuthMech=3;"
                << "UID=token;"
                << "PWD=" << token_str << ";"
                << "SSL=1;"
                << "ThriftTransport=2;";

        // Securely zero the token string
        internal::secure_zero_string(token_str);

        return connStr.str();
    }

    void secure_zero_string(std::string& str) { internal::secure_zero_string(str); }

    std::string sanitize_error_message(const std::string& error) {
        // Redact token from error messages to prevent exposure in logs
        std::string sanitized = error;

        // Get the token to redact (use secure token if available)
        std::string token_to_redact;
        token_to_redact = internal::from_secure_string(auth.get_secure_token());

        // Replace token with [REDACTED] if found
        if (!token_to_redact.empty()) {
            size_t pos = 0;
            while ((pos = sanitized.find(token_to_redact, pos)) != std::string::npos) {
                sanitized.replace(pos, token_to_redact.length(), "[REDACTED]");
                pos += 10; // Length of "[REDACTED]"
            }

            // Securely zero the token copy
            internal::secure_zero_string(token_to_redact);
        }

        // Also redact common token patterns (PWD=..., token=..., etc.)
        // Match PWD=<value>; or PWD=<value> at end of string
        size_t pwd_pos = sanitized.find("PWD=");
        if (pwd_pos != std::string::npos) {
            size_t start = pwd_pos + 4;
            size_t end = sanitized.find(';', start);
            if (end == std::string::npos) {
                end = sanitized.length();
            }
            if (end > start) {
                sanitized.replace(start, end - start, "[REDACTED]");
            }
        }

        return sanitized;
    }

    bool validate_driver_exists() {
        // Enumerate ODBC drivers using the environment handle (no connection needed)
        SQLCHAR driver[256];
        SQLCHAR attributes[256];
        SQLSMALLINT driver_len, attr_len;
        SQLUSMALLINT direction = SQL_FETCH_FIRST;

        while (SQL_SUCCEEDED(SQLDrivers(henv, direction, driver, sizeof(driver), &driver_len, attributes,
                                        sizeof(attributes), &attr_len))) {
            std::string driver_name(reinterpret_cast<char*>(driver));
            if (driver_name == sql.odbc_driver_name) {
                return true;
            }
            direction = SQL_FETCH_NEXT;
        }

        return false;
    }

    void connect() {
        std::lock_guard<std::mutex> lock(connection_mutex);

        if (connected)
            return;

        internal::get_logger()->info("Connecting to Databricks at {}", auth.host);

        // Validate driver exists before attempting connection
        if (!validate_driver_exists()) {
            internal::get_logger()->error("ODBC driver '{}' not found", sql.odbc_driver_name);
            throw std::runtime_error("ODBC driver '" + sql.odbc_driver_name +
                                     "' not found.\n\n"
                                     "To fix this issue:\n"
                                     "1. Download and install the Simba Spark ODBC Driver from:\n"
                                     "   https://www.databricks.com/spark/odbc-drivers-download\n\n"
                                     "2. Verify installation with: odbcinst -q -d\n\n"
                                     "3. If using a different driver, set sql_config.odbc_driver_name to match\n"
                                     "   the driver name shown in odbcinst output.\n");
        }

        SQLCHAR outConnStr[1024];
        SQLSMALLINT outConnStrLen;

        // Build connection string on-demand (not cached)
        std::string conn_str = build_connection_string();

        SQLRETURN ret = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)conn_str.c_str(), SQL_NTS, outConnStr,
                                         sizeof(outConnStr), &outConnStrLen, SQL_DRIVER_NOPROMPT);

        // Securely zero connection string immediately after use
        internal::secure_zero_string(conn_str);

        // Also clear outConnStr buffer (may contain connection details including token)
        volatile unsigned char* p = reinterpret_cast<volatile unsigned char*>(outConnStr);
        for (size_t i = 0; i < sizeof(outConnStr); ++i) {
            p[i] = 0;
        }

        if (!SQL_SUCCEEDED(ret)) {
            std::string error = get_odbc_error(SQL_HANDLE_DBC, hdbc);
            internal::get_logger()->error("Connection failed: {}", sanitize_error_message(error));
            throw std::runtime_error("Failed to connect to Databricks: " + sanitize_error_message(error));
        }

        connected = true;
        internal::get_logger()->info("Successfully connected to {}", auth.host);
    }

    void ensure_connected() {
        // Wait for async connection if in progress
        if (async_connect_future.valid()) {
            async_connect_future.wait();
        }

        // Connect if not already connected
        if (!connected) {
            connect();
        }
    }

    void disconnect() {
        if (connected && hdbc != SQL_NULL_HDBC) {
            internal::get_logger()->info("Disconnecting from Databricks");
            SQLDisconnect(hdbc);
            connected = false;
            internal::get_logger()->debug("Disconnected successfully");
        }
    }

    std::string get_odbc_error(SQLSMALLINT handleType, SQLHANDLE handle) {
        SQLCHAR sqlState[6];
        SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT msgLen;

        std::ostringstream error;
        SQLSMALLINT i = 1;

        while (SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError, message, sizeof(message), &msgLen) ==
               SQL_SUCCESS) {
            error << "[" << sqlState << "] " << message << "; ";
            i++;
        }

        return error.str();
    }

    /**
     * @brief Check if an error message indicates a retryable error
     *
     * Determines if the error is transient and can be retried based on:
     * - ODBC error codes (HY000, 08S01, etc.)
     * - Error message patterns (timeout, connection refused, etc.)
     * - Retry configuration settings
     *
     * This function implements comprehensive error classification for ODBC errors:
     * - Connection errors (08xxx family)
     * - Timeout errors (HYTxx family)
     * - Transient network errors
     * - Server unavailability errors
     *
     * Non-retryable errors include:
     * - Authentication failures
     * - SQL syntax errors
     * - Permission denied errors
     * - Resource exhaustion (out of memory)
     *
     * @param error The error message to check
     * @return true if the error is retryable, false otherwise
     */
    bool is_error_retryable(const std::string& error) const {
        // Connection timeout errors (respects retry_on_timeout config)
        if (retry.retry_on_timeout) {
            if (error.find("timeout") != std::string::npos || error.find("Timeout") != std::string::npos ||
                error.find("TIMEOUT") != std::string::npos ||
                error.find("HYT00") != std::string::npos || // Timeout expired
                error.find("HYT01") != std::string::npos)   // Connection timeout
            {
                return true;
            }
        }

        // Connection lost / network errors (respects retry_on_connection_lost config)
        if (retry.retry_on_connection_lost) {
            if (error.find("Connection refused") != std::string::npos ||
                error.find("Connection reset") != std::string::npos ||
                error.find("Connection lost") != std::string::npos ||
                error.find("Connection closed") != std::string::npos ||
                error.find("Broken pipe") != std::string::npos || error.find("No route to host") != std::string::npos ||
                error.find("Network is unreachable") != std::string::npos ||
                error.find("08S01") != std::string::npos || // Communication link failure
                error.find("08003") != std::string::npos || // Connection does not exist
                error.find("08006") != std::string::npos || // Connection failure
                error.find("08007") != std::string::npos || // Connection failure during transaction
                error.find("08004") != std::string::npos)   // Server rejected connection
            {
                return true;
            }
        }

        // Server temporarily unavailable / transient errors (always retryable)
        if (error.find("Service Unavailable") != std::string::npos ||
            error.find("Too Many Requests") != std::string::npos ||
            error.find("503") != std::string::npos || // HTTP 503 Service Unavailable
            error.find("429") != std::string::npos || // HTTP 429 Too Many Requests
            error.find("502") != std::string::npos || // HTTP 502 Bad Gateway
            error.find("504") != std::string::npos || // HTTP 504 Gateway Timeout
            error.find("HY000") != std::string::npos) // General error (may be transient)
        {
            return true;
        }

        // Non-retryable errors - return false immediately
        if (error.find("28000") != std::string::npos || // Invalid authorization
            error.find("42000") != std::string::npos || // Syntax error or access violation
            error.find("42S02") != std::string::npos || // Table not found
            error.find("42S22") != std::string::npos || // Column not found
            error.find("23000") != std::string::npos || // Integrity constraint violation
            error.find("HY013") != std::string::npos || // Memory allocation error
            error.find("Authentication") != std::string::npos || error.find("Permission denied") != std::string::npos ||
            error.find("Access denied") != std::string::npos) {
            return false;
        }

        return false;
    }

    /**
     * @brief Execute an operation with automatic retry logic
     *
     * Template function that wraps any operation with exponential backoff retry.
     * Automatically retries on transient errors based on RetryConfig settings.
     *
     * Features:
     * - Exponential backoff with configurable multiplier
     * - Jitter added to backoff to avoid thundering herd
     * - Respects max backoff cap to prevent excessive delays
     * - Intelligent error classification (retryable vs non-retryable)
     * - Detailed error messages on final failure
     *
     * @tparam Func Callable type that returns a value
     * @param operation The operation to execute (lambda, function, etc.)
     * @param operation_name Name of the operation for error messages
     * @return The result of the operation
     * @throws std::runtime_error if max retries exceeded or non-retryable error
     */
    template <typename Func>
    auto execute_with_retry(Func&& operation, const std::string& operation_name) -> decltype(operation()) {
        // If retries are disabled, execute directly
        if (!retry.enabled) {
            return operation();
        }

        size_t attempt = 0;
        size_t backoff_ms = retry.initial_backoff_ms;

        while (true) {
            try {
                if (attempt > 0) {
                    internal::get_logger()->debug("Retry attempt {}/{} for {}", attempt + 1, retry.max_attempts,
                                                  operation_name);
                }
                return operation();
            } catch (const std::runtime_error& e) {
                attempt++;
                std::string error_msg = e.what();

                // Check if error is retryable
                bool is_retryable = is_error_retryable(error_msg);

                // If not retryable or max attempts reached, re-throw
                if (!is_retryable || attempt >= retry.max_attempts) {
                    if (attempt >= retry.max_attempts) {
                        std::string sanitized_error = sanitize_error_message(error_msg);
                        internal::get_logger()->error("{} failed after {} attempts: {}", operation_name,
                                                      retry.max_attempts, sanitized_error);
                        throw std::runtime_error("Operation '" + operation_name + "' failed after " +
                                                 std::to_string(attempt) + " attempts: " + sanitized_error);
                    }
                    std::string sanitized_error = sanitize_error_message(error_msg);
                    internal::get_logger()->error("{} failed with non-retryable error: {}", operation_name,
                                                  sanitized_error);
                    throw; // Re-throw non-retryable errors immediately
                }

                // Add jitter (±25%) to prevent thundering herd problem
                // when multiple clients retry simultaneously
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> jitter_dist(0.75, 1.25);
                double jitter = jitter_dist(gen);

                size_t jittered_backoff = static_cast<size_t>(backoff_ms * jitter);

                internal::get_logger()->warn("{} attempt {}/{} failed: {} - retrying in {}ms", operation_name, attempt,
                                             retry.max_attempts, error_msg, jittered_backoff);

                // Sleep with exponential backoff + jitter
                std::this_thread::sleep_for(std::chrono::milliseconds(jittered_backoff));

                // Calculate next backoff with cap (before jitter is applied next time)
                backoff_ms = std::min(static_cast<size_t>(backoff_ms * retry.backoff_multiplier), retry.max_backoff_ms);
            }
        }
    }
};

// ========== Builder Implementation ==========

Client::Builder::Builder() {}

Client::Builder& Client::Builder::with_auth(const AuthConfig& auth) {
    auth_ = std::make_unique<AuthConfig>(auth);
    return *this;
}

Client::Builder& Client::Builder::with_sql(const SQLConfig& sql) {
    sql_ = std::make_unique<SQLConfig>(sql);
    return *this;
}

Client::Builder& Client::Builder::with_pooling(const PoolingConfig& pooling) {
    pooling_ = std::make_unique<PoolingConfig>(pooling);
    return *this;
}

Client::Builder& Client::Builder::with_retry(const RetryConfig& retry) {
    retry_ = std::make_unique<RetryConfig>(retry);
    return *this;
}

Client::Builder& Client::Builder::with_environment_config(const std::string& profile) {
    auto auth = AuthConfig::from_environment(profile);
    auth_ = std::make_unique<AuthConfig>(auth);

    // Try to load http_path from environment
    SQLConfig sql;
    const char* http_path_env = std::getenv("DATABRICKS_HTTP_PATH");
    if (!http_path_env)
        http_path_env = std::getenv("DATABRICKS_SQL_HTTP_PATH");

    if (!http_path_env) {
        // Try to load from profile file
        const char* home = std::getenv("HOME");
        if (home) {
            std::ifstream file(std::string(home) + "/.databrickscfg");
            if (file.is_open()) {
                std::string line;
                bool in_profile_section = false;

                while (std::getline(file, line)) {
                    auto start = line.find_first_not_of(" \t");
                    if (start == std::string::npos)
                        continue;
                    auto end = line.find_last_not_of(" \t");
                    line = line.substr(start, end - start + 1);

                    if (line.empty() || line[0] == '#')
                        continue;

                    if (line.front() == '[' && line.back() == ']') {
                        in_profile_section = (line.substr(1, line.size() - 2) == profile);
                        continue;
                    }

                    if (!in_profile_section)
                        continue;

                    auto pos = line.find('=');
                    if (pos == std::string::npos)
                        continue;

                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);

                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    if (key == "http_path" || key == "sql_http_path") {
                        sql.http_path = value;
                        break;
                    }
                }
            }
        }
    } else {
        sql.http_path = http_path_env;
    }

    if (sql.http_path.empty()) {
        throw std::runtime_error("DATABRICKS_HTTP_PATH not found in environment or profile. "
                                 "Set DATABRICKS_HTTP_PATH environment variable or add http_path to ~/.databrickscfg");
    }

    sql_ = std::make_unique<SQLConfig>(sql);
    return *this;
}

Client::Builder& Client::Builder::with_auto_connect(bool enable) {
    auto_connect_ = enable;
    return *this;
}

Client Client::Builder::build() {
    if (!auth_) {
        throw std::runtime_error("AuthConfig is required. Call with_auth() or with_environment_config()");
    }
    if (!sql_) {
        throw std::runtime_error("SQLConfig is required. Call with_sql() or with_environment_config()");
    }

    PoolingConfig pooling = pooling_ ? *pooling_ : PoolingConfig{};
    RetryConfig retry = retry_ ? *retry_ : RetryConfig{};
    return Client(*auth_, *sql_, pooling, retry, auto_connect_);
}

// ========== Client Implementation ==========

Client::Client(const AuthConfig& auth, const SQLConfig& sql, const PoolingConfig& pooling, const RetryConfig& retry,
               bool auto_connect)
    : pimpl_(std::make_unique<Impl>(auth, sql, pooling, retry, auto_connect)) {}

Client::~Client() = default;

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

const AuthConfig& Client::get_auth_config() const {
    return pimpl_->auth;
}

const SQLConfig& Client::get_sql_config() const {
    return pimpl_->sql;
}

const PoolingConfig& Client::get_pooling_config() const {
    return pimpl_->pooling;
}

bool Client::is_configured() const {
    // Pooled clients are configured if they have a valid pool
    if (pimpl_->pool) {
        return pimpl_->auth.is_valid() && pimpl_->sql.is_valid();
    }

    // Non-pooled clients need to be connected
    return pimpl_->auth.is_valid() && pimpl_->sql.is_valid() && pimpl_->connected;
}

void Client::connect() {
    // Pooled clients don't have a dedicated connection
    if (pimpl_->pool) {
        pimpl_->pool->warm_up();
        return;
    }

    pimpl_->connect();
}

std::future<void> Client::connect_async() {
    // Pooled clients: async warm-up
    if (pimpl_->pool) {
        return pimpl_->pool->warm_up_async();
    }

    // Non-pooled clients: async connect
    auto impl_ptr = pimpl_.get();

    pimpl_->async_connect_future = std::async(std::launch::async, [impl_ptr]() { impl_ptr->connect(); });

    return std::async(std::launch::deferred, [impl_ptr]() {
        if (impl_ptr->async_connect_future.valid()) {
            impl_ptr->async_connect_future.wait();
        }
    });
}

std::future<std::vector<std::vector<std::string>>> Client::query_async(const std::string& sql,
                                                                       const std::vector<Parameter>& params) {
    return std::async(std::launch::async, [this, sql, params]() { return this->query(sql, params); });
}

void Client::disconnect() {
    // Pooled clients don't own connections, so disconnect is a no-op
    if (pimpl_->pool) {
        return;
    }

    pimpl_->disconnect();
}

std::vector<std::vector<std::string>> Client::query(const std::string& sql, const std::vector<Parameter>& params) {
    // Log query execution
    if (internal::get_logger()->should_log(spdlog::level::debug)) {
        std::string query_preview = sql.length() > 100 ? sql.substr(0, 100) + "..." : sql;
        internal::get_logger()->debug("Executing query: {} (params: {})", query_preview, params.size());
    }

    // If pooling is enabled, acquire connection from pool and execute
    if (pimpl_->pool) {
        internal::get_logger()->debug("Using connection pool for query");
        // Wrap pooled query with retry logic
        return pimpl_->execute_with_retry(
            [&]() {
                auto pooled_conn = pimpl_->pool->acquire();
                return pooled_conn->query(sql, params);
                // Connection automatically returns to pool when pooled_conn goes out of scope
            },
            "query");
    }

    // Non-pooled path: use dedicated connection with retry logic
    return pimpl_->execute_with_retry(
        [&]() -> std::vector<std::vector<std::string>> {
            // Ensure connected (lazy connection or wait for async)
            pimpl_->ensure_connected();

            SQLHSTMT hstmt = SQL_NULL_HSTMT;
            SQLRETURN ret;

            // Allocate statement handle
            ret = SQLAllocHandle(SQL_HANDLE_STMT, pimpl_->hdbc, &hstmt);
            if (!SQL_SUCCEEDED(ret)) {
                throw std::runtime_error("Failed to allocate statement handle");
            }

            // Choose execution path based on whether parameters are provided
            if (params.empty()) {
                // Static query - use direct execution for better performance
                ret = SQLExecDirect(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
                if (!SQL_SUCCEEDED(ret)) {
                    std::string error = pimpl_->get_odbc_error(SQL_HANDLE_STMT, hstmt);
                    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    throw std::runtime_error("Query execution failed: " + error);
                }
            } else {
                // Parameterized query - use prepared statement for security
                ret = SQLPrepare(hstmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
                if (!SQL_SUCCEEDED(ret)) {
                    std::string error = pimpl_->get_odbc_error(SQL_HANDLE_STMT, hstmt);
                    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    throw std::runtime_error("Failed to prepare statement: " + error);
                }

                // Bind parameters - store values to prevent dangling pointers
                std::vector<std::string> param_storage;
                std::vector<SQLLEN> param_indicators;
                param_storage.reserve(params.size());
                param_indicators.resize(params.size());

                for (size_t i = 0; i < params.size(); i++) {
                    param_storage.push_back(params[i].value);
                    param_indicators[i] = param_storage[i].length();

                    ret = SQLBindParameter(hstmt,
                                           static_cast<SQLUSMALLINT>(i + 1),     // Parameter number (1-based)
                                           SQL_PARAM_INPUT,                      // Input parameter
                                           params[i].c_type,                     // C data type
                                           params[i].sql_type,                   // SQL data type
                                           param_storage[i].length(),            // Column size
                                           0,                                    // Decimal digits
                                           (SQLPOINTER)param_storage[i].c_str(), // Parameter value pointer
                                           param_storage[i].length(),            // Buffer length
                                           &param_indicators[i]                  // Length/indicator
                    );

                    if (!SQL_SUCCEEDED(ret)) {
                        std::string error = pimpl_->get_odbc_error(SQL_HANDLE_STMT, hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                        throw std::runtime_error("Failed to bind parameter " + std::to_string(i + 1) + ": " + error);
                    }
                }

                // Execute the prepared statement
                ret = SQLExecute(hstmt);
                if (!SQL_SUCCEEDED(ret)) {
                    std::string error = pimpl_->get_odbc_error(SQL_HANDLE_STMT, hstmt);
                    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    throw std::runtime_error("Query execution failed: " + error);
                }
            }

            // Get column count
            SQLSMALLINT colCount = 0;
            ret = SQLNumResultCols(hstmt, &colCount);
            if (!SQL_SUCCEEDED(ret)) {
                SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                throw std::runtime_error("Failed to get column count");
            }

            // Fetch results
            std::vector<std::vector<std::string>> results;

            while (SQLFetch(hstmt) == SQL_SUCCESS) {
                std::vector<std::string> row;

                for (SQLSMALLINT i = 1; i <= colCount; i++) {
                    SQLCHAR buffer[4096];
                    SQLLEN indicator;

                    ret = SQLGetData(hstmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);

                    if (indicator == SQL_NULL_DATA) {
                        row.push_back("");
                    } else if (SQL_SUCCEEDED(ret)) {
                        row.push_back(std::string((char*)buffer));
                    } else {
                        row.push_back("");
                    }
                }

                results.push_back(row);
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            internal::get_logger()->info("Query completed successfully, {} rows returned", results.size());
            return results;
        },
        "query");
}

} // namespace databricks
