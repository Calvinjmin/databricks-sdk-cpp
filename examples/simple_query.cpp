#include <databricks/core/client.h>
#include <iostream>

/**
 * @brief Simple example demonstrating basic SQL query execution
 *
 * This example shows:
 * 1. Creating a client using the Builder pattern
 * 2. Executing a simple query
 * 3. Using parameterized queries for safe input handling
 */
int main() {
    try {
        // Create client from environment configuration
        // This automatically loads from ~/.databrickscfg or environment variables
        auto client = databricks::Client::Builder()
            .with_environment_config()
            .build();

        std::cout << "Connected to Databricks" << std::endl;
        std::cout << std::endl;

        // Example 1: Simple query
        std::cout << "=== Example 1: Simple Query ===" << std::endl;
        auto results = client.query("SELECT current_timestamp() as timestamp, current_user() as user");

        std::cout << "Current timestamp: " << results[0][0] << std::endl;
        std::cout << "Current user: " << results[0][1] << std::endl;
        std::cout << std::endl;

        // Example 2: Parameterized query (RECOMMENDED for user input)
        std::cout << "=== Example 2: Parameterized Query ===" << std::endl;

        // Use parameterized queries to prevent SQL injection
        // The ? placeholders are safely replaced with the parameter values
        std::string user_value = "50";
        std::string sql = "SELECT ? as number, ? * 2 as doubled";
        std::vector<databricks::Client::Parameter> params = {{user_value}, {user_value}};

        std::cout << "Executing: " << sql << std::endl;
        std::cout << "Parameters: ['" << user_value << "', '" << user_value << "']" << std::endl;
        std::cout << std::endl;

        auto data = client.query(sql, params);

        // Print results
        std::cout << "Results:" << std::endl;
        std::cout << "  Number: " << data[0][0] << std::endl;
        std::cout << "  Doubled: " << data[0][1] << std::endl;
        std::cout << std::endl;

        std::cout << "Note: Parameterized queries protect against SQL injection" << std::endl;
        std::cout << "      Always use them when incorporating user input!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << std::endl;
        std::cerr << "To run this example, set the following environment variables:" << std::endl;
        std::cerr << std::endl;
        std::cerr << "export DATABRICKS_HOST=\"https://your-workspace.databricks.com\"" << std::endl;
        std::cerr << "export DATABRICKS_TOKEN=\"your_databricks_token\"" << std::endl;
        std::cerr << "export DATABRICKS_HTTP_PATH=\"/sql/1.0/warehouses/your_warehouse_id\"" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Or configure ~/.databrickscfg with a [DEFAULT] profile." << std::endl;
        return 1;
    }

    return 0;
}
