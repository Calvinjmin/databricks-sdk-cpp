#include <databricks/core/client.h>
#include <iostream>
#include <iomanip>

/**
 * @brief Example demonstrating SECURE parameterized query execution
 *
 * This example shows how to use query() with parameters to prevent SQL injection attacks.
 * Always use parameters when incorporating user input or dynamic values in queries.
 */
int main() {
    try {
        // Use Builder pattern to create client from environment configuration
        auto client = databricks::Client::Builder()
            .with_environment_config()
            .build();

        std::cout << "Connected to Databricks" << std::endl;
        std::cout << "Using Connection Pool: " << (client.get_pooling_config().enabled ? "Yes" : "No") << std::endl;
        std::cout << std::endl;

        // ========== EXAMPLE 1: Basic Parameterized Query ==========
        std::cout << "=== Example 1: Parameterized Query ===" << std::endl;

        // Simulate user input (could come from command line, API, etc.)
        std::string user_input = "The Great Gatsby";

        // SECURE: Use parameterized query with placeholders
        std::string sql = "SELECT * FROM cjm_launchpad.demos.books WHERE title = ?";
        std::vector<databricks::Client::Parameter> params = {{user_input}};

        std::cout << "Executing secure query: " << sql << std::endl;
        std::cout << "Parameter: '" << user_input << "'" << std::endl;

        auto results = client.query(sql, params);

        std::cout << "Results (" << results.size() << " rows):" << std::endl;
        for (size_t i = 0; i < results.size(); i++) {
            std::cout << "  Row " << (i + 1) << ": ";
            for (size_t j = 0; j < results[i].size(); j++) {
                if (j > 0) std::cout << " | ";
                std::cout << results[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        // ========== EXAMPLE 2: Multiple Parameters ==========
        std::cout << "=== Example 2: Multiple Parameters ===" << std::endl;

        std::string author = "F. Scott Fitzgerald";
        std::string min_year = "1920";

        sql = "SELECT * FROM cjm_launchpad.demos.books WHERE author = ? AND year >= ?";
        params = {{author}, {min_year}};

        std::cout << "Executing query: " << sql << std::endl;
        std::cout << "Parameters: ['" << author << "', '" << min_year << "']" << std::endl;

        results = client.query(sql, params);

        std::cout << "Results (" << results.size() << " rows):" << std::endl;
        for (size_t i = 0; i < results.size(); i++) {
            std::cout << "  Row " << (i + 1) << ": ";
            for (size_t j = 0; j < results[i].size(); j++) {
                if (j > 0) std::cout << " | ";
                std::cout << results[i][j];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        // ========== EXAMPLE 3: SQL Injection Attempt (Safely Handled) ==========
        std::cout << "=== Example 3: SQL Injection Attempt (Blocked) ===" << std::endl;

        // This malicious input would cause SQL injection with unsafe query()
        std::string malicious_input = "'; DROP TABLE books; --";

        sql = "SELECT * FROM cjm_launchpad.demos.books WHERE title = ?";
        params = {{malicious_input}};

        std::cout << "Attempting to inject: " << malicious_input << std::endl;
        std::cout << "Using parameterized query (SAFE)" << std::endl;

        // This query will simply search for the literal string "'; DROP TABLE books; --"
        // It will NOT execute the DROP TABLE command
        results = client.query(sql, params);

        std::cout << "Query executed safely. Results: " << results.size() << " rows" << std::endl;
        std::cout << "(The malicious input was treated as a literal string, not SQL code)" << std::endl;
        std::cout << std::endl;

        // ========== COMPARISON: UNSAFE vs SAFE ==========
        std::cout << "=== Comparison: Unsafe vs Safe Patterns ===" << std::endl;
        std::cout << std::endl;
        std::cout << "UNSAFE (String Concatenation - DON'T DO THIS):" << std::endl;
        std::cout << "  std::string sql = \"SELECT * FROM table WHERE id = '\" + user_input + \"'\";" << std::endl;
        std::cout << "  auto results = client.query(sql);" << std::endl;
        std::cout << "  ❌ Vulnerable to SQL injection!" << std::endl;
        std::cout << std::endl;
        std::cout << "SAFE (Parameterized Query - ALWAYS DO THIS):" << std::endl;
        std::cout << "  std::string sql = \"SELECT * FROM table WHERE id = ?\";" << std::endl;
        std::cout << "  std::vector<Client::Parameter> params = {{user_input}};" << std::endl;
        std::cout << "  auto results = client.query(sql, params);" << std::endl;
        std::cout << "  ✅ Protected against SQL injection!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
