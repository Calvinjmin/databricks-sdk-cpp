#include <databricks/client.h>
#include "../common/config_helper.h"
#include <iostream>
#include <chrono>
#include <thread>

/**
 * @brief Example demonstrating asynchronous connection and queries
 *
 * This example shows how to use async operations to reduce perceived latency
 * by performing connection and queries in the background while doing other work.
 */
int main()
{
    try
    {
        std::cout << "=== Async Connection Example ===" << std::endl;
        std::cout << std::endl;

        // Create configuration
        auto config = examples::load_config_from_env();

        // Create client WITHOUT auto-connect (lazy connection)
        std::cout << "Creating client (lazy connection mode)..." << std::endl;
        databricks::Client client(config, false);

        // Start async connection
        std::cout << "Starting async connection..." << std::endl;
        auto start_time = std::chrono::steady_clock::now();
        auto connect_future = client.connect_async();

        // Do other work while connecting in background
        std::cout << "Doing other work while connecting..." << std::endl;
        for (int i = 1; i <= 3; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::cout << "  Background work step " << i << "/3" << std::endl;
        }

        // Wait for connection to complete
        std::cout << "Waiting for connection to complete..." << std::endl;
        connect_future.wait();

        auto connect_time = std::chrono::steady_clock::now();
        auto connect_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              connect_time - start_time)
                              .count();

        std::cout << "Connection completed (total time: " << connect_ms << "ms)" << std::endl;
        std::cout << "Is configured: " << (client.is_configured() ? "Yes" : "No") << std::endl;
        std::cout << std::endl;

        // Execute synchronous query
        std::cout << "=== Synchronous Query ===" << std::endl;
        auto sync_start = std::chrono::steady_clock::now();

        auto sync_results = client.query("SELECT 'Synchronous' as mode, current_timestamp() as ts");

        auto sync_end = std::chrono::steady_clock::now();
        auto sync_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           sync_end - sync_start)
                           .count();

        std::cout << "Query completed in " << sync_ms << "ms" << std::endl;
        std::cout << "Results: ";
        for (const auto &col : sync_results[0])
        {
            std::cout << col << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;

        // Execute asynchronous query
        std::cout << "=== Asynchronous Query ===" << std::endl;
        auto async_start = std::chrono::steady_clock::now();

        std::cout << "Starting async query..." << std::endl;
        auto query_future = client.query_async("SELECT 'Asynchronous' as mode, current_timestamp() as ts");

        // Do other work while query executes
        std::cout << "Doing other work while query executes..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "  Other work completed" << std::endl;

        // Wait for and get results
        std::cout << "Waiting for query results..." << std::endl;
        auto async_results = query_future.get();

        auto async_end = std::chrono::steady_clock::now();
        auto async_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            async_end - async_start)
                            .count();

        std::cout << "Query completed (total time: " << async_ms << "ms)" << std::endl;
        std::cout << "Results: ";
        for (const auto &col : async_results[0])
        {
            std::cout << col << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;

        // Demonstrate multiple concurrent async queries
        std::cout << "=== Multiple Concurrent Async Queries ===" << std::endl;
        auto concurrent_start = std::chrono::steady_clock::now();

        std::cout << "Launching 3 queries concurrently..." << std::endl;
        auto future1 = client.query_async("SELECT 1 as query_id, current_timestamp() as ts");
        auto future2 = client.query_async("SELECT 2 as query_id, current_timestamp() as ts");
        auto future3 = client.query_async("SELECT 3 as query_id, current_timestamp() as ts");

        // Wait for all to complete
        auto result1 = future1.get();
        auto result2 = future2.get();
        auto result3 = future3.get();

        auto concurrent_end = std::chrono::steady_clock::now();
        auto concurrent_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 concurrent_end - concurrent_start)
                                 .count();

        std::cout << "All queries completed in " << concurrent_ms << "ms" << std::endl;
        std::cout << "Result 1: " << result1[0][0] << " at " << result1[0][1] << std::endl;
        std::cout << "Result 2: " << result2[0][0] << " at " << result2[0][1] << std::endl;
        std::cout << "Result 3: " << result3[0][0] << " at " << result3[0][1] << std::endl;

        std::cout << std::endl;
        std::cout << "=== Async example completed successfully ===" << std::endl;
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
