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
            #ifndef _WIN32
            // Set ODBC environment variables for driver discovery
            setenv("ODBCSYSINI", "/opt/homebrew/etc", 1);
            setenv("ODBCINI", "/opt/homebrew/etc/odbc.ini", 1);

            // Set library path for Simba driver to find unixODBC libraries
            setenv("DYLD_LIBRARY_PATH", "/opt/homebrew/lib:/usr/local/lib", 1);
            #endif

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
            connStr << "Driver=Simba Spark ODBC Driver;"
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

        void connect()
        {
            std::lock_guard<std::mutex> lock(connection_mutex);

            if (connected)
                return;

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
