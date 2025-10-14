#include <databricks/core/client.h>
#include "../common/config_helper.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>

/**
 * @brief Benchmark to measure connection pooling performance impact
 *
 * This program runs the same queries with and without pooling to
 * demonstrate the actual performance improvement.
 */

// Helper to measure execution time
template <typename Func>
std::chrono::milliseconds measure_time(Func func)
{
    auto start = std::chrono::steady_clock::now();
    func();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

int main(int argc, char *argv[])
{
    try
    {
        // Configure number of queries to run
        int num_queries = 5;
        if (argc > 1)
        {
            num_queries = std::atoi(argv[1]);
        }

        std::cout << "=== Connection Pooling Performance Benchmark ===" << std::endl;
        std::cout << "Running " << num_queries << " queries in each test" << std::endl;
        std::cout << "(Specify number of queries as argument: ./benchmark_pooling 10)" << std::endl;
        std::cout << std::endl;

        // Simple query for testing
        std::string test_query = "SELECT current_timestamp()";

        std::cout << std::string(70, '=') << std::endl;
        std::cout << "TEST 1: WITHOUT Connection Pooling (baseline)" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Each query creates a new connection..." << std::endl;
        std::cout << std::endl;

        std::vector<long> no_pool_times;

        auto no_pool_total = measure_time([&]()
        {
            for (int i = 0; i < num_queries; i++)
            {
                auto query_time = measure_time([&]()
                {
                    // Create new client each time (simulates new connection overhead)
                    auto client = databricks::Client::Builder().with_environment_config()
                        .with_auto_connect(true)
                        .build();
                    client.query(test_query);
                });

                no_pool_times.push_back(query_time.count());
                std::cout << "  Query " << (i + 1) << ": " << query_time.count() << "ms" << std::endl;
            }
        });

        std::cout << std::endl;
        std::cout << "Total time (no pooling): " << no_pool_total.count() << "ms" << std::endl;
        std::cout << "Average per query: " << (no_pool_total.count() / num_queries) << "ms" << std::endl;
        std::cout << std::endl;

        std::cout << std::string(70, '=') << std::endl;
        std::cout << "TEST 2: WITH Connection Pooling" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Connections are reused from pool..." << std::endl;
        std::cout << std::endl;

        std::vector<long> pool_times;

        // Configure pooling
        databricks::PoolingConfig pooling;
        pooling.enabled = true;
        pooling.min_connections = 2;
        pooling.max_connections = 5;

        // Create client once (pool is shared)
        auto pooled_client = databricks::Client::Builder().with_environment_config()
            .with_pooling(pooling)
            .build();

        // Pre-warm the pool
        std::cout << "Pre-warming pool..." << std::endl;
        pooled_client.connect();
        std::cout << "Pool ready!" << std::endl;
        std::cout << std::endl;

        auto pool_total = measure_time([&]()
        {
            for (int i = 0; i < num_queries; i++)
            {
                auto query_time = measure_time([&]()
                {
                    pooled_client.query(test_query);
                });

                pool_times.push_back(query_time.count());
                std::cout << "  Query " << (i + 1) << ": " << query_time.count() << "ms" << std::endl;
            }
        });

        std::cout << std::endl;
        std::cout << "Total time (with pooling): " << pool_total.count() << "ms" << std::endl;
        std::cout << "Average per query: " << (pool_total.count() / num_queries) << "ms" << std::endl;
        std::cout << std::endl;

        std::cout << std::string(70, '=') << std::endl;
        std::cout << "RESULTS" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << std::endl;

        long time_saved = no_pool_total.count() - pool_total.count();
        double speedup = (double)no_pool_total.count() / pool_total.count();
        double percent_faster = ((double)time_saved / no_pool_total.count()) * 100;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Without pooling: " << no_pool_total.count() << "ms total" << std::endl;
        std::cout << "With pooling:    " << pool_total.count() << "ms total" << std::endl;
        std::cout << std::endl;
        std::cout << "Time saved:      " << time_saved << "ms (" << percent_faster << "% faster)" << std::endl;
        std::cout << "Speedup:         " << speedup << "x" << std::endl;
        std::cout << std::endl;

        // Calculate overhead per connection
        long avg_connection_overhead = (no_pool_total.count() - pool_total.count()) / num_queries;
        std::cout << "Estimated connection overhead: ~" << avg_connection_overhead << "ms per query" << std::endl;
        std::cout << std::endl;

        // Detailed breakdown
        std::cout << "Query-by-query comparison:" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::setw(10) << "Query #"
                  << std::setw(15) << "No Pool (ms)"
                  << std::setw(15) << "Pooled (ms)"
                  << std::setw(12) << "Saved (ms)" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        for (int i = 0; i < num_queries; i++)
        {
            long saved = no_pool_times[i] - pool_times[i];
            std::cout << std::setw(10) << (i + 1)
                      << std::setw(15) << no_pool_times[i]
                      << std::setw(15) << pool_times[i]
                      << std::setw(12) << saved << std::endl;
        }
        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::endl;

        // Verdict
        std::cout << "=== VERDICT ===" << std::endl;
        if (speedup > 2.0)
        {
            std::cout << "✓ Connection pooling provides EXCELLENT performance improvement!" << std::endl;
        }
        else if (speedup > 1.5)
        {
            std::cout << "✓ Connection pooling provides GOOD performance improvement!" << std::endl;
        }
        else if (speedup > 1.1)
        {
            std::cout << "✓ Connection pooling provides measurable performance improvement." << std::endl;
        }
        else
        {
            std::cout << "⚠ Minimal difference - network latency might be dominating." << std::endl;
        }
        std::cout << std::endl;

        std::cout << "Recommendation: ";
        if (speedup > 1.2)
        {
            std::cout << "Enable pooling for applications making multiple queries!" << std::endl;
        }
        else
        {
            std::cout << "Pooling has less impact on your setup, but doesn't hurt." << std::endl;
        }
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
