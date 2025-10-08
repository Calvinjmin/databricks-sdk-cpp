#include <gtest/gtest.h>
#include <databricks/client.h>

/**
 * @brief Test default client construction
 */
TEST(ClientTest, DefaultConstruction) {
    databricks::Client client;
    EXPECT_FALSE(client.is_configured());
}

/**
 * @brief Test client with partial configuration
 */
TEST(ClientTest, PartialConfiguration) {
    databricks::Client::Config config;
    config.host = "https://test.databricks.com";
    config.timeout_seconds = 120;

    databricks::Client client(config);

    const auto& retrieved_config = client.get_config();
    EXPECT_EQ(retrieved_config.host, "https://test.databricks.com");
    EXPECT_EQ(retrieved_config.timeout_seconds, 120);
}

/**
 * @brief Test that invalid credentials throw an error when connecting
 */
TEST(ClientTest, InvalidCredentialsThrow) {
    databricks::Client::Config config;
    config.host = "https://invalid.databricks.com";
    config.token = "invalid_token";
    config.http_path = "/sql/1.0/warehouses/invalid";

    // Client uses lazy connection, so it won't throw until connect() or query()
    databricks::Client client(config);

    EXPECT_THROW({
        client.connect();
    }, std::exception);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
