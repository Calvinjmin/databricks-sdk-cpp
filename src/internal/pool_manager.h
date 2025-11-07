#pragma once

#include "databricks/connection_pool.h"
#include "databricks/core/config.h"
#include "databricks/internal/secure_string.h"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace databricks {
namespace internal {
/**
 * @brief Configuration key for pool sharing
 *
 * This struct is used internally to determine if two clients
 * can share the same connection pool.
 */
struct PoolKey {
    std::string host;
    SecureString token; // Changed to SecureString for security
    std::string http_path;
    int timeout_seconds;
    std::string odbc_driver_name;

    /**
     * @brief Generate hash for pool key
     */
    size_t hash() const;

    /**
     * @brief Check if two pool keys are equivalent
     */
    bool operator==(const PoolKey& other) const;
};

/**
 * @brief Internal singleton managing shared connection pools
 *
 * This class is an implementation detail and should not be used directly.
 * Users should enable pooling via PoolingConfig instead.
 */
class PoolManager {
public:
    /**
     * @brief Get the singleton instance
     */
    static PoolManager& instance();

    /**
     * @brief Get or create a pool for the given configuration
     *
     * Pools are shared across all Clients with equivalent configs.
     * Thread-safe.
     *
     * @param auth Authentication configuration
     * @param sql SQL configuration
     * @param pooling Pooling configuration
     * @return Shared pointer to ConnectionPool
     */
    std::shared_ptr<ConnectionPool> get_pool(const AuthConfig& auth, const SQLConfig& sql,
                                             const PoolingConfig& pooling);

    /**
     * @brief Shutdown all pools
     *
     * Useful for cleanup during application shutdown.
     */
    void shutdown_all();

private:
    PoolManager() = default;
    ~PoolManager() = default;

    // Disable copy and move
    PoolManager(const PoolManager&) = delete;
    PoolManager& operator=(const PoolManager&) = delete;
    PoolManager(PoolManager&&) = delete;
    PoolManager& operator=(PoolManager&&) = delete;

    std::unordered_map<size_t, std::shared_ptr<ConnectionPool>> pools_;
    std::mutex mutex_;
};

} // namespace internal
} // namespace databricks

// Hash function for PoolKey
namespace std {
template <> struct hash<databricks::internal::PoolKey> {
    size_t operator()(const databricks::internal::PoolKey& key) const { return key.hash(); }
};
} // namespace std
