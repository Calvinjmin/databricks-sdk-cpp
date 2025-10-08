#include <databricks/client.h>
#include <databricks/connection_pool.h>
#include "../common/config_helper.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <vector>
#include <future>

/**
 * @brief Example combining connection pooling and async operations
 *
 * This example demonstrates best practices for high-performance Databricks
 * applications by combining connection pooling (for connection reuse) with
 * async operations (for concurrency).
 */
int main()
{
    try
    {
        std::cout << "=== Connection Pool + Async Example ===" << std::endl;
        std::cout << std::endl;

        // Create connection configuration
        auto config = examples::load_config_from_env();

        // Create connection pool
        std::cout << "Creating connection pool (min=3, max=10)..." << std::endl;
        databricks::ConnectionPool pool(config, 3, 10);

        // Async warm-up of the pool
        std::cout << "Starting async pool warm-up..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        auto warmup_future = pool.warm_up_async();

        // Do other initialization work
        std::cout << "Doing other initialization..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  Initialization complete" << std::endl;

        // Wait for warm-up to complete
        warmup_future.wait();
        auto warmup_time = std::chrono::steady_clock::now();
        auto warmup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             warmup_time - start_time)
                             .count();

        std::cout << "Pool warmed up in " << warmup_ms << "ms" << std::endl;

        auto stats = pool.get_stats();
        std::cout << "Pool stats: " << stats.total_connections << " total, "
                  << stats.available_connections << " available" << std::endl;
        std::cout << std::endl;

        // Scenario 1: Sequential queries with pooled connections
        std::cout << "=== Scenario 1: Sequential Queries (Pooled) ===" << std::endl;
        auto seq_start = std::chrono::steady_clock::now();

        for (int i = 1; i <= 3; i++)
        {
            auto client = pool.acquire();
            auto results = client->query("SELECT " + std::to_string(i) + " as seq_num");
            std::cout << "Query " << i << " result: " << results[0][0] << std::endl;
        }

        auto seq_end = std::chrono::steady_clock::now();
        auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          seq_end - seq_start)
                          .count();
        std::cout << "Sequential queries completed in " << seq_ms << "ms" << std::endl;
        std::cout << std::endl;

        // Scenario 2: Concurrent async queries with pooled connections
        std::cout << "=== Scenario 2: Concurrent Async Queries (Pooled) ===" << std::endl;
        auto concurrent_start = std::chrono::steady_clock::now();

        // Launch multiple async queries concurrently
        std::vector<std::future<std::vector<std::vector<std::string>>>> futures;

        std::cout << "Launching 5 concurrent async queries..." << std::endl;
        for (int i = 1; i <= 5; i++)
        {
            // Each async task acquires a connection from pool, executes query, and returns connection
            futures.push_back(std::async(std::launch::async, [&pool, i]()
                                         {
                auto client = pool.acquire();
                return client->query("SELECT " + std::to_string(i) + " as concurrent_num, current_timestamp() as ts"); }));
        }

        // Collect results
        std::cout << "Waiting for all queries to complete..." << std::endl;
        for (size_t i = 0; i < futures.size(); i++)
        {
            auto results = futures[i].get();
            std::cout << "Query " << (i + 1) << " result: " << results[0][0]
                      << " at " << results[0][1] << std::endl;
        }

        auto concurrent_end = std::chrono::steady_clock::now();
        auto concurrent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 concurrent_end - concurrent_start)
                                 .count();
        std::cout << "Concurrent queries completed in " << concurrent_ms << "ms" << std::endl;
        std::cout << std::endl;

        // Show performance improvement
        std::cout << "=== Performance Comparison ===" << std::endl;
        std::cout << "Sequential (3 queries): " << seq_ms << "ms" << std::endl;
        std::cout << "Concurrent (5 queries): " << concurrent_ms << "ms" << std::endl;
        if (concurrent_ms > 0)
        {
            double efficiency = (double)(seq_ms * 5 / 3) / concurrent_ms;
            std::cout << "Efficiency gain: " << std::fixed << std::setprecision(2)
                      << efficiency << "x faster" << std::endl;
        }
        std::cout << std::endl;

        // Final pool statistics
        stats = pool.get_stats();
        std::cout << "Final pool stats:" << std::endl;
        std::cout << "  Total connections: " << stats.total_connections << std::endl;
        std::cout << "  Available: " << stats.available_connections << std::endl;
        std::cout << "  Active: " << stats.active_connections << std::endl;

        std::cout << std::endl;
        std::cout << "=== Pool + Async example completed successfully ===" << std::endl;
        std::cout << std::endl;
        std::cout << "Key Takeaways:" << std::endl;
        std::cout << "  - Connection pooling eliminates connection overhead" << std::endl;
        std::cout << "  - Async operations enable concurrent query execution" << std::endl;
        std::cout << "  - Combined approach provides maximum throughput" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
