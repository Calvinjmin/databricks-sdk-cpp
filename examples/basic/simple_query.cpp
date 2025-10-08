#include <databricks/client.h>
#include "../common/config_helper.h"
#include <iostream>
#include <iomanip>

/**
 * @brief Example demonstrating SQL query execution
 */
int main() {
    try {
        // Load configuration from environment variables
        auto config = examples::load_config_from_env();
        
        // Create a configured client
        databricks::Client client(config);

        std::cout << "Connected to Databricks" << std::endl;
        std::cout << "Using Connection Pool: " << (client.is_configured() ? "Yes" : "No") << std::endl;
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
        examples::print_env_setup_instructions();
        return 1;
    }

    return 0;
}
