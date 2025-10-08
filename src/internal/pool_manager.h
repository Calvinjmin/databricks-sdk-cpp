#pragma once

#include "databricks/client.h"
#include "databricks/connection_pool.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace databricks
{
    namespace internal
    {
        /**
         * @brief Internal singleton managing shared connection pools
         *
         * This class is an implementation detail and should not be used directly.
         * Users should enable pooling via Client::Config instead.
         */
        class PoolManager
        {
        public:
            /**
             * @brief Get the singleton instance
             */
            static PoolManager &instance();

            /**
             * @brief Get or create a pool for the given configuration
             *
             * Pools are shared across all Clients with equivalent configs.
             * Thread-safe.
             *
             * @param config Client configuration
             * @return Shared pointer to ConnectionPool
             */
            std::shared_ptr<ConnectionPool> get_pool(const Client::Config &config);

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
            PoolManager(const PoolManager &) = delete;
            PoolManager &operator=(const PoolManager &) = delete;
            PoolManager(PoolManager &&) = delete;
            PoolManager &operator=(PoolManager &&) = delete;

            std::unordered_map<size_t, std::shared_ptr<ConnectionPool>> pools_;
            std::mutex mutex_;
        };

    } // namespace internal
} // namespace databricks
