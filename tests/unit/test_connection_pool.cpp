#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/connection_pool.h>
#include <databricks/client.h>
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
        pool_config.host = "https://test.databricks.com";
        pool_config.token = "test_token_pool";
        pool_config.http_path = "/sql/1.0/warehouses/test";
        pool_config.enable_pooling = true;
        pool_config.pool_min_connections = 2;
        pool_config.pool_max_connections = 5;
    }

    databricks::Client::Config pool_config;
};

/**
 * @brief Test pool creation with valid config
 */
TEST_F(ConnectionPoolTest, PoolCreation) {
    // Pool creation happens internally when client is created with pooling enabled
    EXPECT_NO_THROW({
        databricks::Client client(pool_config);
    });
}

/**
 * @brief Test client with pooling enabled uses pool
 */
TEST_F(ConnectionPoolTest, ClientUsesPoolWhenEnabled) {
    databricks::Client client(pool_config);

    const auto& config = client.get_config();
    EXPECT_TRUE(config.enable_pooling);
    EXPECT_EQ(config.pool_min_connections, 2);
    EXPECT_EQ(config.pool_max_connections, 5);
}

/**
 * @brief Test multiple clients sharing same pool
 */
TEST_F(ConnectionPoolTest, MultipleClientsSharingPool) {
    databricks::Client client1(pool_config);
    databricks::Client client2(pool_config);

    // Both clients should use the same pool (same config)
    EXPECT_TRUE(client1.get_config().equivalent_for_pooling(client2.get_config()));
}

/**
 * @brief Test pool config equivalence for pooling
 */
TEST_F(ConnectionPoolTest, PoolConfigEquivalence) {
    databricks::Client::Config config1 = pool_config;
    databricks::Client::Config config2 = pool_config;

    // Same connection parameters should be equivalent
    EXPECT_TRUE(config1.equivalent_for_pooling(config2));

    // Different pool settings shouldn't affect equivalence
    config2.pool_max_connections = 10;
    EXPECT_TRUE(config1.equivalent_for_pooling(config2));

    // Different connection parameters should not be equivalent
    config2.host = "https://other.databricks.com";
    EXPECT_FALSE(config1.equivalent_for_pooling(config2));
}

/**
 * @brief Test pool with minimum connections
 */
TEST_F(ConnectionPoolTest, PoolWithMinConnections) {
    pool_config.pool_min_connections = 1;
    pool_config.pool_max_connections = 3;

    databricks::Client client(pool_config);

    EXPECT_EQ(client.get_config().pool_min_connections, 1);
}

/**
 * @brief Test pool with maximum connections
 */
TEST_F(ConnectionPoolTest, PoolWithMaxConnections) {
    pool_config.pool_min_connections = 5;
    pool_config.pool_max_connections = 20;

    databricks::Client client(pool_config);

    EXPECT_EQ(client.get_config().pool_max_connections, 20);
}

/**
 * @brief Test disconnect on pooled client (no-op)
 */
TEST_F(ConnectionPoolTest, DisconnectPooledClient) {
    databricks::Client client(pool_config);

    // Disconnect should be a no-op for pooled clients
    EXPECT_NO_THROW(client.disconnect());
}

/**
 * @brief Test config hash with pooling enabled
 */
TEST_F(ConnectionPoolTest, ConfigHashWithPooling) {
    databricks::Client::Config config1 = pool_config;
    databricks::Client::Config config2 = pool_config;

    // Pooling settings don't affect hash (only connection params)
    config2.pool_max_connections = 100;

    // Hash should be same for equivalent connection parameters
    EXPECT_EQ(config1.hash(), config2.hash());
}
