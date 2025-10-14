#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/connection_pool.h>
#include <databricks/core/client.h>
#include <thread>
#include <vector>
#include <chrono>

using ::testing::_;
using ::testing::Return;

/**
 * @brief Test fixture for ConnectionPool tests
 */
class ConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.token = "test_token_pool";
        auth.timeout_seconds = 60;

        sql.http_path = "/sql/1.0/warehouses/test";

        pooling.enabled = true;
        pooling.min_connections = 2;
        pooling.max_connections = 5;
    }

    databricks::AuthConfig auth;
    databricks::SQLConfig sql;
    databricks::PoolingConfig pooling;
};

/**
 * @brief Test client creation with pooling enabled
 */
TEST_F(ConnectionPoolTest, ClientCreationWithPooling) {
    EXPECT_NO_THROW({
        auto client = databricks::Client::Builder()
            .with_auth(auth)
            .with_sql(sql)
            .with_pooling(pooling)
            .build();
    });
}

/**
 * @brief Test client with pooling enabled uses pool
 */
TEST_F(ConnectionPoolTest, ClientUsesPoolWhenEnabled) {
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    const auto& pool_config = client.get_pooling_config();
    EXPECT_TRUE(pool_config.enabled);
    EXPECT_EQ(pool_config.min_connections, 2);
    EXPECT_EQ(pool_config.max_connections, 5);
}

/**
 * @brief Test multiple clients with same configuration share pool
 */
TEST_F(ConnectionPoolTest, MultipleClientsSharingPool) {
    auto client1 = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    auto client2 = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    // Both clients should have same configuration
    EXPECT_EQ(client1.get_auth_config().host, client2.get_auth_config().host);
    EXPECT_EQ(client1.get_sql_config().http_path, client2.get_sql_config().http_path);
}

/**
 * @brief Test pool with minimum connections
 */
TEST_F(ConnectionPoolTest, PoolWithMinConnections) {
    pooling.min_connections = 1;
    pooling.max_connections = 3;

    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    EXPECT_EQ(client.get_pooling_config().min_connections, 1);
}

/**
 * @brief Test pool with maximum connections
 */
TEST_F(ConnectionPoolTest, PoolWithMaxConnections) {
    pooling.min_connections = 5;
    pooling.max_connections = 20;

    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    EXPECT_EQ(client.get_pooling_config().max_connections, 20);
}

/**
 * @brief Test disconnect on pooled client (no-op)
 */
TEST_F(ConnectionPoolTest, DisconnectPooledClient) {
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .with_pooling(pooling)
        .build();

    // Disconnect should be a no-op for pooled clients
    EXPECT_NO_THROW(client.disconnect());
}

/**
 * @brief Test direct ConnectionPool usage
 */
TEST_F(ConnectionPoolTest, DirectConnectionPoolUsage) {
    EXPECT_NO_THROW({
        databricks::ConnectionPool pool(auth, sql, 2, 5);
        auto stats = pool.get_stats();
        // Initially, no connections should be created yet
        EXPECT_EQ(stats.total_connections, 0);
    });
}

/**
 * @brief Test PoolingConfig validation
 */
TEST_F(ConnectionPoolTest, PoolingConfigValidation) {
    // Valid config
    EXPECT_TRUE(pooling.is_valid());

    // Zero min_connections is invalid
    databricks::PoolingConfig invalid_pooling;
    invalid_pooling.min_connections = 0;
    EXPECT_FALSE(invalid_pooling.is_valid());

    // max < min is invalid
    invalid_pooling.min_connections = 10;
    invalid_pooling.max_connections = 5;
    EXPECT_FALSE(invalid_pooling.is_valid());
}

