#include "databricks/client.h"
#include "databricks/connection_pool.h"
#include "internal/pool_manager.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <future>
#include <mutex>
#include <functional>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

namespace databricks
{
    // ========== Config Implementation ==========

    size_t Client::Config::hash() const
    {
        // Simple hash combining relevant fields for pooling
        std::hash<std::string> hasher;
        size_t h = hasher(host);
        h ^= hasher(token) << 1;
        h ^= hasher(http_path) << 2;
        h ^= std::hash<int>()(timeout_seconds) << 3;
        return h;
    }

    bool Client::Config::equivalent_for_pooling(const Config &other) const
    {
        return host == other.host &&
               token == other.token &&
               http_path == other.http_path &&
               timeout_seconds == other.timeout_seconds;
    }

    bool Client::Config::load_profile_config(const std::string& profile) {
        const char* home = std::getenv("HOME");
        if (!home) return false;

        std::ifstream file(std::string(home) + "/.databrickscfg");
        if (!file.is_open()) return false;

        std::string line;
        bool in_profile = false;

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

            if (key == "host") host = value;
            else if (key == "token") token = value;
            else if (key == "http_path" || key == "sql_http_path") http_path = value;
        }

        return !host.empty() && !token.empty() && !http_path.empty();
    }

    bool Client::Config::load_from_env() {
        bool found_all_required = true;

        // Load host
        const char* host_env = std::getenv("DATABRICKS_HOST");
        if (!host_env) host_env = std::getenv("DATABRICKS_SERVER_HOSTNAME");
        if (host_env) {
            host = host_env;
        } else {
            found_all_required = false;
        }

        // Load token
        const char* token_env = std::getenv("DATABRICKS_TOKEN");
        if (!token_env) token_env = std::getenv("DATABRICKS_ACCESS_TOKEN");
        if (token_env) {
            token = token_env;
        } else {
            found_all_required = false;
        }

        // Load HTTP path
        const char* http_path_env = std::getenv("DATABRICKS_HTTP_PATH");
        if (!http_path_env) http_path_env = std::getenv("DATABRICKS_SQL_HTTP_PATH");
        if (http_path_env) {
            http_path = http_path_env;
        } else {
            found_all_required = false;
        }

        // Load optional timeout
        const char* timeout_env = std::getenv("DATABRICKS_TIMEOUT");
        if (timeout_env) {
            timeout_seconds = std::atoi(timeout_env);
        }

        return found_all_required;
    }

    Client::Config Client::Config::from_environment(const std::string& profile) {
        Config config;

        // First, try to load from profile - if successful, use it and stop
        bool profile_loaded = config.load_profile_config(profile);
        if (profile_loaded) {
            // Profile has all required configs, use it exclusively
            return config;
        }

        // Profile failed or incomplete, fall back to environment variables
        const char* host_env = std::getenv("DATABRICKS_HOST");
        if (!host_env) host_env = std::getenv("DATABRICKS_SERVER_HOSTNAME");
        if (host_env) config.host = host_env;

        const char* token_env = std::getenv("DATABRICKS_TOKEN");
        if (!token_env) token_env = std::getenv("DATABRICKS_ACCESS_TOKEN");
        if (token_env) config.token = token_env;

        const char* http_path_env = std::getenv("DATABRICKS_HTTP_PATH");
        if (!http_path_env) http_path_env = std::getenv("DATABRICKS_SQL_HTTP_PATH");
        if (http_path_env) config.http_path = http_path_env;

        const char* timeout_env = std::getenv("DATABRICKS_TIMEOUT");
        if (timeout_env) config.timeout_seconds = std::atoi(timeout_env);

        // Validate that we have all required fields from env vars
        if (config.host.empty() || config.token.empty() || config.http_path.empty()) {
            throw std::runtime_error(
                "Failed to load Databricks configuration. Ensure either:\n"
                "  1. ~/.databrickscfg has a [" + profile + "] section with host, token, and http_path/sql_http_path, OR\n"
                "  2. Environment variables are set: DATABRICKS_HOST, DATABRICKS_TOKEN, and DATABRICKS_HTTP_PATH"
            );
        }

        return config;
    }

    // ========== Client::Impl ==========

    /**
     * @brief Private implementation class (PIMPL pattern)
     */
    class Client::Impl
    {
    public:
        Config config;
        SQLHENV henv; // Environment handle
        SQLHDBC hdbc; // Connection handle
        bool connected;
        std::string cached_connection_string; // Pre-built connection string
        std::mutex connection_mutex; // Thread safety for connection operations
        std::future<void> async_connect_future; // Track async connection
        std::shared_ptr<ConnectionPool> pool; // Shared pool (if pooling enabled)

        explicit Impl(const Config &cfg, bool auto_connect)
            : config(cfg), henv(SQL_NULL_HENV), hdbc(SQL_NULL_HDBC), connected(false), pool(nullptr)
        {
            // If pooling is enabled, get/create shared pool and return early
            // Pool-based clients don't maintain their own connection
            if (config.enable_pooling)
            {
                pool = internal::PoolManager::instance().get_pool(config);
                // Optionally warm up the pool asynchronously
                if (auto_connect)
                {
                    pool->warm_up_async();
                }
                return; // Don't allocate ODBC handles for pooled clients
            }

            // Non-pooled client: allocate dedicated ODBC connection
            // Note: ODBC driver manager (unixODBC) will use the system's configured
            // odbcinst.ini and odbc.ini files. Users should configure these properly
            // or set ODBCSYSINI/ODBCINI environment variables before running.

            // Allocate environment handle
            SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
            if (!SQL_SUCCEEDED(ret))
            {
                throw std::runtime_error("Failed to allocate ODBC environment handle");
            }

            // Set ODBC version
            ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
            if (!SQL_SUCCEEDED(ret))
            {
                SQLFreeHandle(SQL_HANDLE_ENV, henv);
                throw std::runtime_error("Failed to set ODBC version");
            }

            // Allocate connection handle
            ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
            if (!SQL_SUCCEEDED(ret))
            {
                SQLFreeHandle(SQL_HANDLE_ENV, henv);
                throw std::runtime_error("Failed to allocate ODBC connection handle");
            }

            // Set connection timeouts for better performance
            SQLSetConnectAttr(hdbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);
            SQLSetConnectAttr(hdbc, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)30, 0);

            // Pre-build connection string if config is provided
            if (!config.host.empty() && !config.token.empty() && !config.http_path.empty())
            {
                cached_connection_string = build_connection_string();

                if (auto_connect)
                {
                    connect();
                }
            }
        }

        ~Impl()
        {
            // Pooled clients don't own ODBC handles
            if (pool)
            {
                return;
            }

            // Non-pooled clients: clean up owned resources
            disconnect();
            if (hdbc != SQL_NULL_HDBC)
            {
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
            if (henv != SQL_NULL_HENV)
            {
                SQLFreeHandle(SQL_HANDLE_ENV, henv);
            }
        }

        std::string build_connection_string()
        {
            // Strip https:// or http:// from host if present
            std::string host = config.host;
            if (host.find("https://") == 0) {
                host = host.substr(8);
            } else if (host.find("http://") == 0) {
                host = host.substr(7);
            }

            // Build connection string (pre-cache for performance)
            std::ostringstream connStr;
            connStr << "Driver=" << config.odbc_driver_name << ";"
                    << "Host=" << host << ";"
                    << "Port=443;"
                    << "HTTPPath=" << config.http_path << ";"
                    << "AuthMech=3;"
                    << "UID=token;"
                    << "PWD=" << config.token << ";"
                    << "SSL=1;"
                    << "ThriftTransport=2;";

            return connStr.str();
        }

        bool validate_driver_exists()
        {
            SQLHSTMT hstmt = SQL_NULL_HSTMT;
            SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
            if (!SQL_SUCCEEDED(ret))
            {
                return false;
            }

            SQLCHAR driver[256];
            SQLCHAR attributes[256];
            SQLSMALLINT driver_len, attr_len;
            SQLUSMALLINT direction = SQL_FETCH_FIRST;

            while (SQL_SUCCEEDED(SQLDrivers(henv, direction, driver, sizeof(driver), &driver_len,
                                           attributes, sizeof(attributes), &attr_len)))
            {
                std::string driver_name(reinterpret_cast<char*>(driver));
                if (driver_name == config.odbc_driver_name)
                {
                    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    return true;
                }
                direction = SQL_FETCH_NEXT;
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            return false;
        }

        void connect()
        {
            std::lock_guard<std::mutex> lock(connection_mutex);

            if (connected)
                return;

            // Validate driver exists before attempting connection
            if (!validate_driver_exists())
            {
                throw std::runtime_error(
                    "ODBC driver '" + config.odbc_driver_name + "' not found.\n\n"
                    "To fix this issue:\n"
                    "1. Download and install the Simba Spark ODBC Driver from:\n"
                    "   https://www.databricks.com/spark/odbc-drivers-download\n\n"
                    "2. Verify installation with: odbcinst -q -d\n\n"
                    "3. If using a different driver, set config.odbc_driver_name to match\n"
                    "   the driver name shown in odbcinst output.\n"
                );
            }

            // Use cached connection string if available, otherwise build it
            std::string connString = cached_connection_string.empty()
                ? build_connection_string()
                : cached_connection_string;

            SQLCHAR outConnStr[1024];
            SQLSMALLINT outConnStrLen;

            SQLRETURN ret = SQLDriverConnect(
                hdbc,
                NULL,
                (SQLCHAR *)connString.c_str(),
                SQL_NTS,
                outConnStr,
                sizeof(outConnStr),
                &outConnStrLen,
                SQL_DRIVER_NOPROMPT);

            if (!SQL_SUCCEEDED(ret))
            {
                std::string error = get_odbc_error(SQL_HANDLE_DBC, hdbc);
                throw std::runtime_error("Failed to connect to Databricks: " + error);
            }

            connected = true;
        }

        void ensure_connected()
        {
            // Wait for async connection if in progress
            if (async_connect_future.valid())
            {
                async_connect_future.wait();
            }

            // Connect if not already connected
            if (!connected)
            {
                connect();
            }
        }

        void disconnect()
        {
            if (connected && hdbc != SQL_NULL_HDBC)
            {
                SQLDisconnect(hdbc);
                connected = false;
            }
        }

        std::string get_odbc_error(SQLSMALLINT handleType, SQLHANDLE handle)
        {
            SQLCHAR sqlState[6];
            SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
            SQLINTEGER nativeError;
            SQLSMALLINT msgLen;

            std::ostringstream error;
            SQLSMALLINT i = 1;

            while (SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError,
                                 message, sizeof(message), &msgLen) == SQL_SUCCESS)
            {
                error << "[" << sqlState << "] " << message << "; ";
                i++;
            }

            return error.str();
        }
    };

    Client::Client()
        : pimpl_(std::make_unique<Impl>(Config{}, false))
    {
    }

    Client::Client(const Config &config, bool auto_connect)
        : pimpl_(std::make_unique<Impl>(config, auto_connect))
    {
    }

    Client::~Client() = default;

    Client::Client(Client &&) noexcept = default;
    Client &Client::operator=(Client &&) noexcept = default;

    const Client::Config &Client::get_config() const
    {
        return pimpl_->config;
    }

    bool Client::is_configured() const
    {
        // Pooled clients are configured if they have a valid pool
        if (pimpl_->pool)
        {
            return !pimpl_->config.host.empty() &&
                   !pimpl_->config.token.empty() &&
                   !pimpl_->config.http_path.empty();
        }

        // Non-pooled clients need to be connected
        return !pimpl_->config.host.empty() &&
               !pimpl_->config.token.empty() &&
               !pimpl_->config.http_path.empty() &&
               pimpl_->connected;
    }

    void Client::connect()
    {
        // Pooled clients don't have a dedicated connection
        if (pimpl_->pool)
        {
            pimpl_->pool->warm_up();
            return;
        }

        pimpl_->connect();
    }

    std::future<void> Client::connect_async()
    {
        // Pooled clients: async warm-up
        if (pimpl_->pool)
        {
            return pimpl_->pool->warm_up_async();
        }

        // Non-pooled clients: async connect
        auto impl_ptr = pimpl_.get();

        pimpl_->async_connect_future = std::async(std::launch::async, [impl_ptr]()
        {
            impl_ptr->connect();
        });

        return std::async(std::launch::deferred, [impl_ptr]()
        {
            if (impl_ptr->async_connect_future.valid())
            {
                impl_ptr->async_connect_future.wait();
            }
        });
    }

    std::future<std::vector<std::vector<std::string>>> Client::query_async(const std::string &sql)
    {
        return std::async(std::launch::async, [this, sql]()
        {
            return this->query(sql);
        });
    }

    void Client::disconnect()
    {
        // Pooled clients don't own connections, so disconnect is a no-op
        if (pimpl_->pool)
        {
            return;
        }

        pimpl_->disconnect();
    }

    std::vector<std::vector<std::string>> Client::query(const std::string &sql)
    {
        // If pooling is enabled, acquire connection from pool and execute
        if (pimpl_->pool)
        {
            auto pooled_conn = pimpl_->pool->acquire();
            return pooled_conn->query(sql);
            // Connection automatically returns to pool when pooled_conn goes out of scope
        }

        // Non-pooled path: use dedicated connection
        // Ensure connected (lazy connection or wait for async)
        pimpl_->ensure_connected();

        SQLHSTMT hstmt = SQL_NULL_HSTMT;
        SQLRETURN ret;

        // Allocate statement handle
        ret = SQLAllocHandle(SQL_HANDLE_STMT, pimpl_->hdbc, &hstmt);
        if (!SQL_SUCCEEDED(ret))
        {
            throw std::runtime_error("Failed to allocate statement handle");
        }

        // Execute query
        ret = SQLExecDirect(hstmt, (SQLCHAR *)sql.c_str(), SQL_NTS);
        if (!SQL_SUCCEEDED(ret))
        {
            std::string error = pimpl_->get_odbc_error(SQL_HANDLE_STMT, hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            throw std::runtime_error("Query execution failed: " + error);
        }

        // Get column count
        SQLSMALLINT colCount = 0;
        ret = SQLNumResultCols(hstmt, &colCount);
        if (!SQL_SUCCEEDED(ret))
        {
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
            throw std::runtime_error("Failed to get column count");
        }

        // Fetch results
        std::vector<std::vector<std::string>> results;

        while (SQLFetch(hstmt) == SQL_SUCCESS)
        {
            std::vector<std::string> row;

            for (SQLSMALLINT i = 1; i <= colCount; i++)
            {
                SQLCHAR buffer[4096];
                SQLLEN indicator;

                ret = SQLGetData(hstmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);

                if (indicator == SQL_NULL_DATA)
                {
                    row.push_back("");
                }
                else if (SQL_SUCCEEDED(ret))
                {
                    row.push_back(std::string((char *)buffer));
                }
                else
                {
                    row.push_back("");
                }
            }

            results.push_back(row);
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return results;
    }

} // namespace databricks
