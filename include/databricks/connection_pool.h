#pragma once

#include "databricks/core/config.h"
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>

// Forward declare Client to avoid circular dependency
namespace databricks {
    class Client;
}

namespace databricks
{
    /**
     * @brief Thread-safe connection pool for managing Databricks connections
     *
     * **ADVANCED API**: Most users should enable pooling via PoolingConfig instead.
     *
     * This class is for advanced users who need fine-grained control over connection
     * pooling. For typical use cases, simply set `pooling.enabled = true` when
     * building a Client, and pooling will be handled automatically.
     *
     * The ConnectionPool manages a pool of reusable ODBC connections to improve
     * performance by eliminating connection overhead for repeated operations.
     */
    class ConnectionPool
    {
    public:
        /**
         * @brief RAII wrapper for a pooled connection
         *
         * Automatically returns the connection to the pool when destroyed.
         */
        class PooledConnection
        {
        public:
            PooledConnection(std::unique_ptr<Client> client, ConnectionPool *pool);
            ~PooledConnection();

            // Disable copy
            PooledConnection(const PooledConnection &) = delete;
            PooledConnection &operator=(const PooledConnection &) = delete;

            // Enable move
            PooledConnection(PooledConnection &&) noexcept;
            PooledConnection &operator=(PooledConnection &&) noexcept;

            /**
             * @brief Get the underlying client
             * @return Reference to the Client
             */
            Client &get();

            /**
             * @brief Get the underlying client (const version)
             * @return Const reference to the Client
             */
            const Client &get() const;

            /**
             * @brief Access the client via pointer semantics
             */
            Client *operator->();

            /**
             * @brief Access the client via pointer semantics (const)
             */
            const Client *operator->() const;

        private:
            std::unique_ptr<Client> client_;
            ConnectionPool *pool_;
        };

        /**
         * @brief Construct a ConnectionPool with modular configurations
         * @param auth Authentication configuration
         * @param sql SQL configuration
         * @param min_connections Minimum connections to maintain
         * @param max_connections Maximum connections allowed
         */
        ConnectionPool(const AuthConfig& auth,
                      const SQLConfig& sql,
                      size_t min_connections = 1,
                      size_t max_connections = 10);

        /**
         * @brief Destructor - closes all connections
         */
        ~ConnectionPool();

        // Disable copy
        ConnectionPool(const ConnectionPool &) = delete;
        ConnectionPool &operator=(const ConnectionPool &) = delete;

        // Disable move (pool should be a long-lived object)
        ConnectionPool(ConnectionPool &&) = delete;
        ConnectionPool &operator=(ConnectionPool &&) = delete;

        /**
         * @brief Acquire a connection from the pool
         *
         * If no connections are available and the pool is not at max capacity,
         * a new connection is created. Otherwise, waits for a connection to
         * become available (up to connection_timeout_ms).
         *
         * @return PooledConnection RAII wrapper
         * @throws std::runtime_error if timeout occurs or connection fails
         */
        PooledConnection acquire();

        /**
         * @brief Pre-warm the pool by creating minimum connections
         *
         * Creates min_connections in the pool without blocking. Useful for
         * reducing initial latency.
         */
        void warm_up();

        /**
         * @brief Asynchronously pre-warm the pool
         *
         * Creates min_connections in the background.
         *
         * @return std::future<void> Future that completes when warm-up is done
         */
        std::future<void> warm_up_async();

        /**
         * @brief Get current pool statistics
         */
        struct Stats
        {
            size_t total_connections;     ///< Total connections created
            size_t available_connections; ///< Connections currently available
            size_t active_connections;    ///< Connections currently in use
        };

        /**
         * @brief Get current pool statistics
         * @return Stats structure with current pool state
         */
        Stats get_stats() const;

        /**
         * @brief Shutdown the pool and close all connections
         */
        void shutdown();

    private:
        AuthConfig auth_;
        SQLConfig sql_;
        size_t min_connections_;
        size_t max_connections_;
        int connection_timeout_ms_;

        std::queue<std::unique_ptr<Client>> available_connections_;
        size_t total_connections_;
        size_t active_connections_;
        bool shutdown_;

        mutable std::mutex mutex_;
        std::condition_variable cv_;

        /**
         * @brief Create a new connection (must be called with mutex held)
         */
        std::unique_ptr<Client> create_connection();

        /**
         * @brief Return a connection to the pool
         */
        void return_connection(std::unique_ptr<Client> client);

        friend class PooledConnection;
    };

} // namespace databricks
