#include "databricks/connection_pool.h"
#include "databricks/client.h"
#include <stdexcept>
#include <chrono>

namespace databricks
{

    // ========== PooledConnection Implementation ==========

    ConnectionPool::PooledConnection::PooledConnection(std::unique_ptr<Client> client, ConnectionPool *pool)
        : client_(std::move(client)), pool_(pool)
    {
    }

    ConnectionPool::PooledConnection::~PooledConnection()
    {
        if (client_ && pool_)
        {
            pool_->return_connection(std::move(client_));
        }
    }

    ConnectionPool::PooledConnection::PooledConnection(PooledConnection &&other) noexcept
        : client_(std::move(other.client_)), pool_(other.pool_)
    {
        other.pool_ = nullptr;
    }

    ConnectionPool::PooledConnection &ConnectionPool::PooledConnection::operator=(PooledConnection &&other) noexcept
    {
        if (this != &other)
        {
            // Return current client to pool before taking ownership of new one
            if (client_ && pool_)
            {
                pool_->return_connection(std::move(client_));
            }

            client_ = std::move(other.client_);
            pool_ = other.pool_;
            other.pool_ = nullptr;
        }
        return *this;
    }

    Client &ConnectionPool::PooledConnection::get()
    {
        if (!client_)
        {
            throw std::runtime_error("PooledConnection: client is null");
        }
        return *client_;
    }

    const Client &ConnectionPool::PooledConnection::get() const
    {
        if (!client_)
        {
            throw std::runtime_error("PooledConnection: client is null");
        }
        return *client_;
    }

    Client *ConnectionPool::PooledConnection::operator->()
    {
        return client_.get();
    }

    const Client *ConnectionPool::PooledConnection::operator->() const
    {
        return client_.get();
    }

    // ========== ConnectionPool Implementation ==========

    ConnectionPool::ConnectionPool(const AuthConfig& auth,
                                   const SQLConfig& sql,
                                   size_t min_connections,
                                   size_t max_connections)
        : auth_(auth),
          sql_(sql),
          min_connections_(min_connections),
          max_connections_(max_connections),
          connection_timeout_ms_(5000),
          total_connections_(0),
          active_connections_(0),
          shutdown_(false)
    {
        if (min_connections_ > max_connections_)
        {
            throw std::invalid_argument("min_connections cannot exceed max_connections");
        }
    }

    ConnectionPool::~ConnectionPool()
    {
        shutdown();
    }

    std::unique_ptr<Client> ConnectionPool::create_connection()
    {
        // This should be called with mutex held
        auto client = Client::Builder()
            .with_auth(auth_)
            .with_sql(sql_)
            .with_auto_connect(true)
            .build();
        total_connections_++;
        return std::make_unique<Client>(std::move(client));
    }

    ConnectionPool::PooledConnection ConnectionPool::acquire()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for available connection or ability to create new one
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(connection_timeout_ms_);

        while (true)
        {
            if (shutdown_)
            {
                throw std::runtime_error("ConnectionPool has been shut down");
            }

            // Try to get an available connection
            if (!available_connections_.empty())
            {
                auto client = std::move(available_connections_.front());
                available_connections_.pop();
                active_connections_++;
                return PooledConnection(std::move(client), this);
            }

            // Create a new connection if under max limit
            if (total_connections_ < max_connections_)
            {
                auto client = create_connection();
                active_connections_++;
                return PooledConnection(std::move(client), this);
            }

            // Wait for a connection to become available
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout)
            {
                throw std::runtime_error("Timeout waiting for connection from pool");
            }
        }
    }

    void ConnectionPool::return_connection(std::unique_ptr<Client> client)
    {
        if (!client)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (shutdown_)
        {
            // Don't return connections if shutting down
            total_connections_--;
            return;
        }

        active_connections_--;
        available_connections_.push(std::move(client));

        // Notify one waiting thread
        cv_.notify_one();
    }

    void ConnectionPool::warm_up()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (shutdown_)
        {
            throw std::runtime_error("Cannot warm up: pool is shut down");
        }

        // Create connections up to min_connections
        while (total_connections_ < min_connections_)
        {
            auto client = create_connection();
            available_connections_.push(std::move(client));
        }
    }

    std::future<void> ConnectionPool::warm_up_async()
    {
        return std::async(std::launch::async, [this]()
                          { this->warm_up(); });
    }

    ConnectionPool::Stats ConnectionPool::get_stats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return Stats{
            total_connections_,
            available_connections_.size(),
            active_connections_};
    }

    void ConnectionPool::shutdown()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (shutdown_)
        {
            return;
        }

        shutdown_ = true;

        // Clear all available connections
        while (!available_connections_.empty())
        {
            available_connections_.pop();
            total_connections_--;
        }

        // Wake up all waiting threads so they can throw
        cv_.notify_all();
    }

} // namespace databricks
