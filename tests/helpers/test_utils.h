#pragma once

#include <string>

namespace databricks {
namespace test {
/**
 * @brief Helper utilities for testing
 */
class TestUtils {
public:
    /**
     * @brief Get test configuration from environment
     *
     * Reads DATABRICKS_* environment variables for testing.
     * Falls back to default values if not set.
     */
    static struct TestConfig {
        std::string host;
        std::string token;
        std::string http_path;
    } get_test_config();

    /**
     * @brief Check if integration tests should run
     *
     * Integration tests require real Databricks credentials.
     * Set DATABRICKS_RUN_INTEGRATION_TESTS=1 to enable.
     */
    static bool should_run_integration_tests();
};

} // namespace test
} // namespace databricks
