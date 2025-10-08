#include "pool_manager.h"

namespace databricks
{
    namespace internal
    {
        PoolManager &PoolManager::instance()
        {
            static PoolManager instance;
            return instance;
        }

        std::shared_ptr<ConnectionPool> PoolManager::get_pool(const Client::Config &config)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            size_t config_hash = config.hash();

            // Check if pool already exists
            auto it = pools_.find(config_hash);
            if (it != pools_.end())
            {
                return it->second;
            }

            // Create new pool with pooling DISABLED for internal clients
            // This prevents circular dependency where pool clients try to use the pool
            Client::Config pool_config = config;
            pool_config.enable_pooling = false; // CRITICAL: disable pooling for pool's internal clients

            auto pool = std::make_shared<ConnectionPool>(
                pool_config,
                config.pool_min_connections,
                config.pool_max_connections);

            pools_[config_hash] = pool;
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
