#include "pool_manager.h"
#include <functional>

namespace databricks
{
    namespace internal
    {
        // ========== PoolKey Implementation ==========

        size_t PoolKey::hash() const
        {
            std::hash<std::string> hasher;
            size_t h = hasher(host);
            h ^= hasher(token) << 1;
            h ^= hasher(http_path) << 2;
            h ^= std::hash<int>()(timeout_seconds) << 3;
            h ^= hasher(odbc_driver_name) << 4;
            return h;
        }

        bool PoolKey::operator==(const PoolKey& other) const
        {
            return host == other.host &&
                   token == other.token &&
                   http_path == other.http_path &&
                   timeout_seconds == other.timeout_seconds &&
                   odbc_driver_name == other.odbc_driver_name;
        }

        // ========== PoolManager Implementation ==========

        PoolManager &PoolManager::instance()
        {
            static PoolManager instance;
            return instance;
        }

        std::shared_ptr<ConnectionPool> PoolManager::get_pool(
            const AuthConfig& auth,
            const SQLConfig& sql,
            const PoolingConfig& pooling)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // Create pool key from configuration
            PoolKey key{
                auth.host,
                auth.token,
                sql.http_path,
                auth.timeout_seconds,
                sql.odbc_driver_name
            };

            size_t key_hash = key.hash();

            // Check if pool already exists
            auto it = pools_.find(key_hash);
            if (it != pools_.end())
            {
                return it->second;
            }

            // Create new pool - pass modular configs instead of legacy Config
            // The ConnectionPool will need to be updated to accept modular configs
            auto pool = std::make_shared<ConnectionPool>(
                auth,
                sql,
                pooling.min_connections,
                pooling.max_connections
            );

            pools_[key_hash] = pool;
            return pool;
        }

        void PoolManager::shutdown_all()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto &pair : pools_)
            {
                pair.second->shutdown();
            }
            pools_.clear();
        }

    } // namespace internal
} // namespace databricks
