#include <databricks/core/client.h>
#include <databricks/core/config.h>
#include <gtest/gtest.h>

/**
 * @brief Test client construction with Builder and invalid config
 */
TEST(ClientTest, BuilderWithInvalidConfig) {
    // Creating a client without auth config should throw
    EXPECT_THROW({ databricks::Client::Builder().build(); }, std::runtime_error);
}

/**
 * @brief Test client with complete configuration using Builder
 */
TEST(ClientTest, BuilderConstruction) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");
    auth.timeout_seconds = 120;

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    // This should not throw since we have all required fields
    auto client = databricks::Client::Builder().with_auth(auth).with_sql(sql).build();

    const auto& retrieved_auth = client.get_auth_config();
    const auto& retrieved_sql = client.get_sql_config();

    EXPECT_EQ(retrieved_auth.host, "https://test.databricks.com");
    EXPECT_TRUE(retrieved_auth.has_secure_token());
    EXPECT_EQ(retrieved_auth.timeout_seconds, 120);
    EXPECT_EQ(retrieved_sql.http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test that invalid credentials throw an error when connecting
 */
TEST(ClientTest, InvalidCredentialsThrow) {
    databricks::AuthConfig auth;
    auth.host = "https://invalid.databricks.com";
    auth.set_token("invalid_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/invalid";

    // Client uses lazy connection, so it won't throw until connect() or query()
    auto client = databricks::Client::Builder().with_auth(auth).with_sql(sql).build();

    EXPECT_THROW({ client.connect(); }, std::exception);
}

/**
 * @brief Test client with retry configuration enabled using Builder
 */
TEST(ClientTest, RetryConstructionEnabled) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    databricks::RetryConfig retry;
    retry.enabled = true;
    retry.max_attempts = 5;
    retry.initial_backoff_ms = 200;
    retry.backoff_multiplier = 3.0;
    retry.max_backoff_ms = 15000;

    auto client = databricks::Client::Builder().with_auth(auth).with_sql(sql).with_retry(retry).build();

    // Verify configuration was applied correctly
    EXPECT_EQ(client.get_auth_config().host, "https://test.databricks.com");
    EXPECT_EQ(client.get_sql_config().http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test client with retry configuration disabled using Builder
 */
TEST(ClientTest, RetryConstructionDisabled) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    databricks::RetryConfig retry;
    retry.enabled = false;

    auto client = databricks::Client::Builder().with_auth(auth).with_sql(sql).with_retry(retry).build();

    // Verify client was constructed successfully
    EXPECT_EQ(client.get_auth_config().host, "https://test.databricks.com");
    EXPECT_EQ(client.get_sql_config().http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test retry config validation
 */
TEST(ClientTest, RetryConfigValidation) {
    databricks::RetryConfig valid_retry;
    valid_retry.max_attempts = 3;
    valid_retry.initial_backoff_ms = 100;
    valid_retry.backoff_multiplier = 2.0;
    valid_retry.max_backoff_ms = 10000;
    EXPECT_TRUE(valid_retry.is_valid());

    // Invalid: max_attempts = 0
    databricks::RetryConfig invalid_attempts;
    invalid_attempts.max_attempts = 0;
    invalid_attempts.initial_backoff_ms = 100;
    invalid_attempts.backoff_multiplier = 2.0;
    invalid_attempts.max_backoff_ms = 10000;
    EXPECT_FALSE(invalid_attempts.is_valid());

    // Invalid: initial_backoff_ms = 0
    databricks::RetryConfig invalid_backoff;
    invalid_backoff.max_attempts = 3;
    invalid_backoff.initial_backoff_ms = 0;
    invalid_backoff.backoff_multiplier = 2.0;
    invalid_backoff.max_backoff_ms = 10000;
    EXPECT_FALSE(invalid_backoff.is_valid());

    // Invalid: backoff_multiplier = 0
    databricks::RetryConfig invalid_multiplier;
    invalid_multiplier.max_attempts = 3;
    invalid_multiplier.initial_backoff_ms = 100;
    invalid_multiplier.backoff_multiplier = 0.0;
    invalid_multiplier.max_backoff_ms = 10000;
    EXPECT_FALSE(invalid_multiplier.is_valid());

    // Invalid: max_backoff_ms < initial_backoff_ms
    databricks::RetryConfig invalid_max_backoff;
    invalid_max_backoff.max_attempts = 3;
    invalid_max_backoff.initial_backoff_ms = 10000;
    invalid_max_backoff.backoff_multiplier = 2.0;
    invalid_max_backoff.max_backoff_ms = 100;
    EXPECT_FALSE(invalid_max_backoff.is_valid());
}

/**
 * @brief Test retry config defaults
 */
TEST(ClientTest, RetryConfigDefaults) {
    databricks::RetryConfig retry;

    EXPECT_TRUE(retry.enabled);
    EXPECT_EQ(retry.max_attempts, 3);
    EXPECT_EQ(retry.initial_backoff_ms, 100);
    EXPECT_EQ(retry.backoff_multiplier, 2.0);
    EXPECT_EQ(retry.max_backoff_ms, 10000);
    EXPECT_TRUE(retry.retry_on_timeout);
    EXPECT_TRUE(retry.retry_on_connection_lost);
    EXPECT_TRUE(retry.is_valid());
}

/**
 * @brief Test client with pooling and retry configuration combined
 */
TEST(ClientTest, PoolingAndRetryConfiguration) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    databricks::PoolingConfig pooling;
    pooling.enabled = true;
    pooling.min_connections = 2;
    pooling.max_connections = 5;

    databricks::RetryConfig retry;
    retry.enabled = true;
    retry.max_attempts = 4;

    auto client =
        databricks::Client::Builder().with_auth(auth).with_sql(sql).with_pooling(pooling).with_retry(retry).build();

    const auto& retrieved_pooling = client.get_pooling_config();
    EXPECT_TRUE(retrieved_pooling.enabled);
    EXPECT_EQ(retrieved_pooling.min_connections, 2);
    EXPECT_EQ(retrieved_pooling.max_connections, 5);
}

/**
 * @brief Test client move semantics
 */
TEST(ClientTest, MoveSemantics) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    auto client1 = databricks::Client::Builder().with_auth(auth).with_sql(sql).build();

    // Move constructor
    auto client2 = std::move(client1);
    EXPECT_EQ(client2.get_auth_config().host, "https://test.databricks.com");

    // Move assignment
    auto client3 = databricks::Client::Builder().with_auth(auth).with_sql(sql).build();

    client3 = std::move(client2);
    EXPECT_EQ(client3.get_auth_config().host, "https://test.databricks.com");
}

/**
 * @brief Test builder chaining with all configurations
 */
TEST(ClientTest, BuilderChaining) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";

    databricks::PoolingConfig pooling;
    pooling.enabled = true;

    databricks::RetryConfig retry;
    retry.enabled = true;
    retry.max_attempts = 5;

    // Test fluent chaining of all builder methods
    auto client = databricks::Client::Builder()
                      .with_auth(auth)
                      .with_sql(sql)
                      .with_pooling(pooling)
                      .with_retry(retry)
                      .with_auto_connect(false)
                      .build();

    EXPECT_EQ(client.get_auth_config().host, "https://test.databricks.com");
    EXPECT_EQ(client.get_sql_config().http_path, "/sql/1.0/warehouses/test");
    EXPECT_TRUE(client.get_pooling_config().enabled);
}

/**
 * @brief Test that missing SQL config throws
 */
TEST(ClientTest, MissingSQLConfig) {
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");

    EXPECT_THROW({ databricks::Client::Builder().with_auth(auth).build(); }, std::runtime_error);
}