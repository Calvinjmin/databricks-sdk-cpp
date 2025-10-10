#include <databricks/client.h>
#include <iostream>

/**
 * @brief Example demonstrating modular configuration and Builder pattern
 *
 * This example shows multiple ways to configure the Databricks client:
 * 1. Using the Builder with environment configuration (recommended for most cases)
 * 2. Using the Builder with explicit configuration (for fine-grained control)
 * 3. Accessing modular configuration from the client
 */
int main() {
    try {
        std::cout << "=== Example 1: Builder with Environment Configuration (Simplest) ===" << std::endl;
        std::cout << "This is the recommended approach for most applications." << std::endl;
        std::cout << std::endl;

        // Simplest approach: load from environment/profile
        auto client1 = databricks::Client::Builder()
            .with_environment_config()  // Loads from ~/.databrickscfg or environment variables
            .build();

        auto results1 = client1.query("SELECT * FROM cjm_launchpad.demos.books LIMIT 3");
        std::cout << "Query returned " << results1.size() << " rows" << std::endl;
        std::cout << std::endl;

        std::cout << "=== Example 2: Builder with Explicit Configuration (Advanced) ===" << std::endl;
        std::cout << "Use this when you need fine-grained control over configuration." << std::endl;
        std::cout << std::endl;

        // Advanced: explicit configuration
        databricks::AuthConfig auth;
        auth.host = std::getenv("DATABRICKS_HOST") ? std::getenv("DATABRICKS_HOST") : "";
        auth.token = std::getenv("DATABRICKS_TOKEN") ? std::getenv("DATABRICKS_TOKEN") : "";
        auth.timeout_seconds = 90;  // Custom timeout

        databricks::SQLConfig sql;
        sql.http_path = std::getenv("DATABRICKS_HTTP_PATH") ? std::getenv("DATABRICKS_HTTP_PATH") : "";
        sql.odbc_driver_name = "Simba Spark ODBC Driver";

        databricks::PoolingConfig pooling;
        pooling.enabled = false;  // Disable pooling for this example
        pooling.max_connections = 20;

        auto client2 = databricks::Client::Builder()
            .with_auth(auth)
            .with_sql(sql)
            .with_pooling(pooling)
            .build();

        auto results2 = client2.query("SELECT COUNT(*) as total FROM cjm_launchpad.demos.books");
        std::cout << "Total books: " << results2[0][0] << std::endl;
        std::cout << std::endl;

        std::cout << "=== Example 3: Accessing Modular Configuration ===" << std::endl;
        std::cout << "The client exposes separate configuration objects." << std::endl;
        std::cout << std::endl;

        const auto& auth_config = client1.get_auth_config();
        const auto& sql_config = client1.get_sql_config();
        const auto& pooling_config = client1.get_pooling_config();

        std::cout << "Auth Config:" << std::endl;
        std::cout << "  Host: " << auth_config.host << std::endl;
        std::cout << "  Timeout: " << auth_config.timeout_seconds << "s" << std::endl;
        std::cout << std::endl;

        std::cout << "SQL Config:" << std::endl;
        std::cout << "  HTTP Path: " << sql_config.http_path << std::endl;
        std::cout << "  ODBC Driver: " << sql_config.odbc_driver_name << std::endl;
        std::cout << std::endl;

        std::cout << "Pooling Config:" << std::endl;
        std::cout << "  Enabled: " << (pooling_config.enabled ? "Yes" : "No") << std::endl;
        std::cout << "  Max Connections: " << pooling_config.max_connections << std::endl;
        std::cout << std::endl;

        std::cout << "✅ All examples completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
