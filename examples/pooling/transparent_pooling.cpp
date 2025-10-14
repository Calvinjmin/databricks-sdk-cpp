#include <databricks/core/client.h>
#include "../common/config_helper.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>

/**
 * @brief Example demonstrating transparent connection pooling
 *
 * This example shows how easy it is to enable connection pooling.
 * Simply set pooling.enabled = true using the Builder, and the SDK handles the rest!
 */
int main()
{
    try
    {
        std::cout << "=== Connection Pooling Example ===" << std::endl;
        std::cout << std::endl;

        // Configure pooling settings
        databricks::PoolingConfig pooling;
        pooling.enabled = true;
        pooling.min_connections = 2;
        pooling.max_connections = 5;

        std::cout << "Creating clients with pooling enabled..." << std::endl;
        std::cout << "Pool settings: min=2, max=5" << std::endl;
        std::cout << std::endl;

        // Create multiple clients - they automatically share the same pool
        auto client1 = databricks::Client::Builder().with_environment_config()
            .with_pooling(pooling)
            .build();

        auto client2 = databricks::Client::Builder().with_environment_config()
            .with_pooling(pooling)
            .build();

        auto client3 = databricks::Client::Builder().with_environment_config()
            .with_pooling(pooling)
            .build();

        std::cout << "All clients ready! Pooling happens automatically." << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        std::cout << std::endl;

        // Execute queries - pooling is completely transparent
        std::cout << "Executing queries (connections acquired/released automatically)..." << std::endl;
        std::cout << std::endl;

        for (int i = 1; i <= 5; i++)
        {
            auto start = std::chrono::steady_clock::now();

            // Pick a client (they all use the same pool internally)
            databricks::Client& client = (i % 3 == 0) ? client1 : (i % 3 == 1) ? client2 : client3;

            // Just call query() - connection is acquired from pool, used, and returned automatically
            auto results = client.query("SELECT " + std::to_string(i) + " as query_num, current_timestamp() as ts");

            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            std::cout << "Query " << i << " (client" << ((i % 3) + 1) << "): "
                      << results[0][0] << " | " << results[0][1]
                      << " (" << ms << "ms)" << std::endl;
        }

        std::cout << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        std::cout << std::endl;

        std::cout << "=== Key Benefits ===" << std::endl;
        std::cout << "✓ Simple API - just use Builder with pooling config" << std::endl;
        std::cout << "✓ Automatic pooling - no manual pool management" << std::endl;
        std::cout << "✓ Shared pools - multiple Clients share connections" << std::endl;
        std::cout << "✓ Performance - 10-100x faster than creating new connections" << std::endl;
        std::cout << std::endl;

        std::cout << "=== Comparison: Pooled vs Non-Pooled ===" << std::endl;
        std::cout << std::endl;

        // Demonstrate performance difference
        std::cout << "Testing 3 queries WITHOUT pooling..." << std::endl;

        auto no_pool_start = std::chrono::steady_clock::now();
        for (int i = 0; i < 3; i++)
        {
            auto temp_client = databricks::Client::Builder().with_environment_config()
                .with_auto_connect(true)
                .build();
            temp_client.query("SELECT 1");
        }
        auto no_pool_end = std::chrono::steady_clock::now();
        auto no_pool_ms = std::chrono::duration_cast<std::chrono::milliseconds>(no_pool_end - no_pool_start).count();

        std::cout << "Without pooling: " << no_pool_ms << "ms for 3 queries" << std::endl;
        std::cout << std::endl;

        std::cout << "Testing 3 queries WITH pooling..." << std::endl;
        auto pool_start = std::chrono::steady_clock::now();
        for (int i = 0; i < 3; i++)
        {
            client1.query("SELECT 1");
        }
        auto pool_end = std::chrono::steady_clock::now();
        auto pool_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pool_end - pool_start).count();

        std::cout << "With pooling: " << pool_ms << "ms for 3 queries" << std::endl;
        std::cout << std::endl;

        if (pool_ms > 0)
        {
            double speedup = (double)no_pool_ms / pool_ms;
            std::cout << "Speedup: " << std::fixed << std::setprecision(1) << speedup << "x faster!" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "=== Example completed successfully ===" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << std::endl;
        examples::print_env_setup_instructions();
        return 1;
    }

    return 0;
}
