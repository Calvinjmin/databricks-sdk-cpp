#include <gtest/gtest.h>
#include <databricks/client.h>

/**
 * @brief Test client construction with Builder and invalid config
 */
TEST(ClientTest, BuilderWithInvalidConfig) {
    // Creating a client without auth config should throw
    EXPECT_THROW({
        databricks::Client::Builder()
            .build();
    }, std::runtime_error);
}

/**
 * @brief Test client with complete configuration using Builder
 */
TEST(ClientTest, BuilderConstruction) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.token = "test_token";
    auth.timeout_seconds = 120;

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    // This should not throw since we have all required fields
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .build();

    const auto& retrieved_auth = client.get_auth_config();
    const auto& retrieved_sql = client.get_sql_config();

    EXPECT_EQ(retrieved_auth.host, "https://test.databricks.com");
    EXPECT_EQ(retrieved_auth.token, "test_token");
    EXPECT_EQ(retrieved_auth.timeout_seconds, 120);
    EXPECT_EQ(retrieved_sql.http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test that invalid credentials throw an error when connecting
 */
TEST(ClientTest, InvalidCredentialsThrow) {
    databricks::AuthConfig auth;
    auth.host = "https://invalid.databricks.com";
    auth.token = "invalid_token";

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/invalid";

    // Client uses lazy connection, so it won't throw until connect() or query()
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .build();

    EXPECT_THROW({
        client.connect();
    }, std::exception);
}

