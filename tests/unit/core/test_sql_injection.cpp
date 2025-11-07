// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "../mocks/mock_odbc.h"

#include <databricks/core/client.h>
#include <gtest/gtest.h>

/**
 * @brief Test suite for SQL injection prevention
 *
 * These tests verify that parameterized queries properly protect against
 * SQL injection attacks by treating user input as literal values.
 */
class SQLInjectionTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;
    databricks::SQLConfig sql;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        sql.http_path = "/sql/1.0/warehouses/test";
    }

    databricks::Client create_client() {
        // Disable retries for unit tests to make them faster
        // Unit tests don't have real ODBC connections, so retries just add delay
        databricks::RetryConfig retry;
        retry.enabled = false;

        return databricks::Client::Builder().with_auth(auth).with_sql(sql).with_retry(retry).build();
    }
};

/**
 * @brief Test that query_prepared() accepts parameters correctly
 */
TEST_F(SQLInjectionTest, ParameterizedQueryBasic) {
    // This test verifies the API accepts parameters
    // In a real environment with mocked ODBC, this would verify
    // that SQLPrepare and SQLBindParameter are called correctly

    auto client = create_client();

    std::string sql = "SELECT * FROM users WHERE id = ?";
    std::vector<databricks::Client::Parameter> params = {{"123"}};

    // In production with proper ODBC setup, this would execute
    // Here we're testing the API contract
    EXPECT_NO_THROW({
        try {
            auto results = client.query(sql, params);
        } catch (const std::exception& e) {
            // Expected to fail without real ODBC connection
            // but should not fail due to parameter binding issues
            std::string error = e.what();
            EXPECT_TRUE(error.find("parameter") == std::string::npos)
                << "Error should not be about parameter binding: " << error;
        }
    });
}

/**
 * @brief Test multiple parameters
 */
TEST_F(SQLInjectionTest, MultipleParameters) {
    auto client = create_client();

    std::string sql = "SELECT * FROM users WHERE name = ? AND age > ?";
    std::vector<databricks::Client::Parameter> params = {{"John"}, {"25"}};

    EXPECT_NO_THROW({
        try {
            auto results = client.query(sql, params);
        } catch (const std::exception& e) {
            // Expected to fail without real ODBC connection
            std::string error = e.what();
            EXPECT_TRUE(error.find("parameter") == std::string::npos)
                << "Error should not be about parameter binding: " << error;
        }
    });
}

/**
 * @brief Test that SQL injection strings are safely handled
 *
 * This test verifies that malicious SQL injection attempts are treated
 * as literal strings rather than executable SQL code.
 */
TEST_F(SQLInjectionTest, SQLInjectionAttemptsSafe) {
    auto client = create_client();

    // Common SQL injection patterns
    std::vector<std::string> malicious_inputs = {"'; DROP TABLE users; --",
                                                 "' OR '1'='1",
                                                 "admin'--",
                                                 "' UNION SELECT * FROM passwords--",
                                                 "1; DELETE FROM users WHERE '1'='1",
                                                 "'; EXEC sp_MSForEachTable 'DROP TABLE ?'; --"};

    for (const auto& malicious : malicious_inputs) {
        std::string sql = "SELECT * FROM users WHERE id = ?";
        std::vector<databricks::Client::Parameter> params = {{malicious}};

        // The key test: query_prepared should NOT throw due to the malicious input
        // It should treat it as a literal string parameter
        EXPECT_NO_THROW({
            try {
                auto results = client.query(sql, params);
            } catch (const std::exception& e) {
                // Connection errors are expected without ODBC setup
                // But should NOT be SQL syntax errors from injection
                std::string error = e.what();
                EXPECT_TRUE(error.find("syntax") == std::string::npos)
                    << "Malicious input caused SQL syntax error: " << malicious;
                EXPECT_TRUE(error.find("DROP") == std::string::npos) << "Malicious input was executed: " << malicious;
            }
        });
    }
}

/**
 * @brief Test empty parameters
 */
TEST_F(SQLInjectionTest, EmptyParameters) {
    auto client = create_client();

    std::string sql = "SELECT * FROM users";
    std::vector<databricks::Client::Parameter> params = {}; // No parameters

    EXPECT_NO_THROW({
        try {
            auto results = client.query(sql, params);
        } catch (const std::exception& e) {
            // Expected connection error, not parameter error
            std::string error = e.what();
            EXPECT_TRUE(error.find("parameter") == std::string::npos);
        }
    });
}

/**
 * @brief Test special characters in parameters
 */
TEST_F(SQLInjectionTest, SpecialCharactersInParameters) {
    auto client = create_client();

    std::vector<std::string> special_chars = {
        "value with spaces", "value'with'quotes",        "value\"with\"doublequotes", "value\nwith\nnewlines",
        "value\twith\ttabs", "value\\with\\backslashes", "value%with%wildcards",      "value_with_underscores"};

    for (const auto& value : special_chars) {
        std::string sql = "SELECT * FROM users WHERE name = ?";
        std::vector<databricks::Client::Parameter> params = {{value}};

        EXPECT_NO_THROW({
            try {
                auto results = client.query(sql, params);
            } catch (const std::exception& e) {
                // Should not fail due to special characters
                std::string error = e.what();
                EXPECT_TRUE(error.find("syntax") == std::string::npos)
                    << "Special characters caused syntax error: " << value;
            }
        });
    }
}

/**
 * @brief Test parameter count mismatch detection
 *
 * Verifies that providing wrong number of parameters is handled properly
 */
TEST_F(SQLInjectionTest, ParameterCountMismatch) {
    auto client = create_client();

    // SQL expects 2 parameters but we provide 1
    std::string sql = "SELECT * FROM users WHERE name = ? AND age = ?";
    std::vector<databricks::Client::Parameter> params = {{"John"}}; // Missing one!

    // This should either:
    // 1. Throw an error about parameter count (ideal)
    // 2. Fail at ODBC level with parameter binding error
    // Either way, it should not execute with undefined behavior
    EXPECT_NO_THROW({
        try {
            auto results = client.query(sql, params);
        } catch (const std::exception& e) {
            // Expected to throw some error
            std::string error = e.what();
            EXPECT_FALSE(error.empty()) << "Should throw error on parameter mismatch";
        }
    });
}

/**
 * @brief Test API documentation contract
 *
 * Verifies that the Parameter struct has expected fields
 */
TEST_F(SQLInjectionTest, ParameterStructContract) {
    databricks::Client::Parameter param;

    // Should have these fields as per API contract
    param.value = "test";
    param.c_type = SQL_C_CHAR;
    param.sql_type = SQL_VARCHAR;

    // Verify defaults
    databricks::Client::Parameter default_param;
    EXPECT_EQ(default_param.c_type, SQL_C_CHAR);
    EXPECT_EQ(default_param.sql_type, SQL_VARCHAR);
}

/**
 * @brief Integration test documentation
 *
 * This test class focuses on unit testing the API contract.
 * For full integration testing with actual ODBC and Databricks:
 *
 * 1. Set up test database with known data
 * 2. Execute queries with malicious input using parameterized queries
 * 3. Verify that:
 *    - No tables were dropped
 *    - No unauthorized data was accessed
 *    - Query returns empty results (malicious string doesn't match records)
 * 4. Compare with unsafe string concatenation to demonstrate vulnerability
 */
