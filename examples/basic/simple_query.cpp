#include <databricks/core/client.h>
#include <iostream>
#include <iomanip>

/**
 * @brief Example demonstrating SQL query execution with the new Builder pattern
 *
 * This example shows the recommended way to create a client using the Builder pattern.
 */
int main() {
    try {
        // NEW: Use Builder pattern to create client from environment configuration
        // This automatically loads from ~/.databrickscfg or environment variables
        auto client = databricks::Client::Builder()
            .with_environment_config()
            .build();

        std::cout << "Connected to Databricks" << std::endl;
        std::cout << "Using Connection Pool: " << (client.get_pooling_config().enabled ? "Yes" : "No") << std::endl;
        std::cout << std::endl;

        // Execute a simple query
        std::string dbx_table = "cjm_launchpad.demos.books";
        std::string sql = "SELECT * FROM " + dbx_table;
        std::cout << "Executing query: " << sql << std::endl;
        std::cout << std::endl;

        auto results = client.query(sql);

        // Print results
        std::cout << "Results (" << results.size() << " rows):" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        for (size_t i = 0; i < results.size(); i++) {
            std::cout << "Row " << (i + 1) << ": ";
            for (size_t j = 0; j < results[i].size(); j++) {
                if (j > 0) std::cout << " | ";
                std::cout << results[i][j];
            }
            std::cout << std::endl;
        }

        std::cout << std::string(60, '-') << std::endl;

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
