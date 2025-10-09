#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/client.h>
#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Test fixture for Config tests with environment setup/teardown
 */
class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save existing environment variables
        saved_home = getenv_safe("HOME");
        saved_host = getenv_safe("DATABRICKS_HOST");
        saved_token = getenv_safe("DATABRICKS_TOKEN");
        saved_http_path = getenv_safe("DATABRICKS_HTTP_PATH");
        saved_timeout = getenv_safe("DATABRICKS_TIMEOUT");

        // Create temporary directory for test config files
        temp_dir = fs::temp_directory_path() / ("databricks_test_" + std::to_string(time(nullptr)));
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        // Restore environment variables
        restore_env("HOME", saved_home);
        restore_env("DATABRICKS_HOST", saved_host);
        restore_env("DATABRICKS_TOKEN", saved_token);
        restore_env("DATABRICKS_HTTP_PATH", saved_http_path);
        restore_env("DATABRICKS_TIMEOUT", saved_timeout);

        // Clean up temporary directory
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    }

    std::string getenv_safe(const char* name) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : "";
    }

    void restore_env(const char* name, const std::string& value) {
        if (value.empty()) {
            unsetenv(name);
        } else {
            setenv(name, value.c_str(), 1);
        }
    }

    void create_config_file(const std::string& content) {
        std::string config_path = temp_dir / ".databrickscfg";
        std::ofstream file(config_path);
        file << content;
        file.close();
        setenv("HOME", temp_dir.c_str(), 1);
    }

    std::string saved_home, saved_host, saved_token, saved_http_path, saved_timeout;
    fs::path temp_dir;
};

/**
 * @brief Test Config default construction
 */
TEST_F(ConfigTest, DefaultConstruction) {
    databricks::Client::Config config;
    EXPECT_TRUE(config.host.empty());
    EXPECT_TRUE(config.token.empty());
    EXPECT_TRUE(config.http_path.empty());
    EXPECT_EQ(config.timeout_seconds, 60);
    EXPECT_FALSE(config.enable_pooling);
    EXPECT_EQ(config.pool_min_connections, 1);
    EXPECT_EQ(config.pool_max_connections, 10);
}

/**
 * @brief Test Config construction with values
 */
TEST_F(ConfigTest, ConstructionWithValues) {
    databricks::Client::Config config;
    config.host = "https://test.databricks.com";
    config.token = "test_token";
    config.http_path = "/sql/1.0/warehouses/test";
    config.timeout_seconds = 120;
    config.enable_pooling = true;
    config.pool_max_connections = 20;

    EXPECT_EQ(config.host, "https://test.databricks.com");
    EXPECT_EQ(config.token, "test_token");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/test");
    EXPECT_EQ(config.timeout_seconds, 120);
    EXPECT_TRUE(config.enable_pooling);
    EXPECT_EQ(config.pool_max_connections, 20);
}

/**
 * @brief Test Config hash generation
 */
TEST_F(ConfigTest, HashGeneration) {
    databricks::Client::Config config1;
    config1.host = "https://test.databricks.com";
    config1.token = "token123";
    config1.http_path = "/sql/1.0/warehouses/test";
    config1.timeout_seconds = 60;

    databricks::Client::Config config2;
    config2.host = "https://test.databricks.com";
    config2.token = "token123";
    config2.http_path = "/sql/1.0/warehouses/test";
    config2.timeout_seconds = 60;

    // Same configs should produce same hash
    EXPECT_EQ(config1.hash(), config2.hash());

    // Different configs should produce different hash (with high probability)
    config2.token = "different_token";
    EXPECT_NE(config1.hash(), config2.hash());
}

/**
 * @brief Test Config equivalence for pooling
 */
TEST_F(ConfigTest, EquivalenceForPooling) {
    databricks::Client::Config config1;
    config1.host = "https://test.databricks.com";
    config1.token = "token123";
    config1.http_path = "/sql/1.0/warehouses/test";
    config1.timeout_seconds = 60;

    databricks::Client::Config config2;
    config2.host = "https://test.databricks.com";
    config2.token = "token123";
    config2.http_path = "/sql/1.0/warehouses/test";
    config2.timeout_seconds = 60;

    EXPECT_TRUE(config1.equivalent_for_pooling(config2));

    // Change host
    config2.host = "https://other.databricks.com";
    EXPECT_FALSE(config1.equivalent_for_pooling(config2));
    config2.host = config1.host;

    // Change token
    config2.token = "different_token";
    EXPECT_FALSE(config1.equivalent_for_pooling(config2));
    config2.token = config1.token;

    // Change http_path
    config2.http_path = "/sql/1.0/warehouses/other";
    EXPECT_FALSE(config1.equivalent_for_pooling(config2));
    config2.http_path = config1.http_path;

    // Change timeout
    config2.timeout_seconds = 120;
    EXPECT_FALSE(config1.equivalent_for_pooling(config2));

    // Pooling settings should not affect equivalence
    config2.timeout_seconds = config1.timeout_seconds;
    config2.enable_pooling = !config1.enable_pooling;
    EXPECT_TRUE(config1.equivalent_for_pooling(config2));
}

/**
 * @brief Test loading config from profile with DEFAULT section
 */
TEST_F(ConfigTest, LoadProfileConfigDefault) {
    std::string config_content = R"(
[DEFAULT]
host = https://profile.databricks.com
token = profile_token_123
http_path = /sql/1.0/warehouses/profile_warehouse
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("DEFAULT");

    EXPECT_TRUE(result);
    EXPECT_EQ(config.host, "https://profile.databricks.com");
    EXPECT_EQ(config.token, "profile_token_123");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/profile_warehouse");
}

/**
 * @brief Test loading config from named profile
 */
TEST_F(ConfigTest, LoadProfileConfigNamed) {
    std::string config_content = R"(
[DEFAULT]
host = https://default.databricks.com
token = default_token

[production]
host = https://production.databricks.com
token = prod_token_456
http_path = /sql/1.0/warehouses/prod_warehouse

[staging]
host = https://staging.databricks.com
token = staging_token
http_path = /sql/1.0/warehouses/staging_warehouse
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("production");

    EXPECT_TRUE(result);
    EXPECT_EQ(config.host, "https://production.databricks.com");
    EXPECT_EQ(config.token, "prod_token_456");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/prod_warehouse");
}

/**
 * @brief Test loading config with sql_http_path alternative key
 */
TEST_F(ConfigTest, LoadProfileConfigWithSqlHttpPath) {
    std::string config_content = R"(
[DEFAULT]
host = https://test.databricks.com
token = test_token
sql_http_path = /sql/1.0/warehouses/test
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("DEFAULT");

    EXPECT_TRUE(result);
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test loading config with comments and whitespace
 */
TEST_F(ConfigTest, LoadProfileConfigWithCommentsAndWhitespace) {
    std::string config_content = R"(
# This is a comment
[DEFAULT]
  host   =   https://test.databricks.com
  token  =   test_token_with_spaces
  # Another comment
  http_path = /sql/1.0/warehouses/test
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("DEFAULT");

    EXPECT_TRUE(result);
    EXPECT_EQ(config.host, "https://test.databricks.com");
    EXPECT_EQ(config.token, "test_token_with_spaces");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/test");
}

/**
 * @brief Test loading non-existent profile
 */
TEST_F(ConfigTest, LoadProfileConfigNonExistent) {
    std::string config_content = R"(
[DEFAULT]
host = https://test.databricks.com
token = test_token
http_path = /sql/1.0/warehouses/test
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("nonexistent");

    EXPECT_FALSE(result);
}

/**
 * @brief Test loading profile with missing required fields
 */
TEST_F(ConfigTest, LoadProfileConfigIncomplete) {
    std::string config_content = R"(
[DEFAULT]
host = https://test.databricks.com
token = test_token
# http_path is missing
)";
    create_config_file(config_content);

    databricks::Client::Config config;
    bool result = config.load_profile_config("DEFAULT");

    EXPECT_FALSE(result);
}

/**
 * @brief Test loading profile when config file doesn't exist
 */
TEST_F(ConfigTest, LoadProfileConfigFileNotFound) {
    setenv("HOME", "/nonexistent/path", 1);

    databricks::Client::Config config;
    bool result = config.load_profile_config("DEFAULT");

    EXPECT_FALSE(result);
}

/**
 * @brief Test loading config from environment variables
 */
TEST_F(ConfigTest, LoadFromEnvBasic) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);
    
    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token_789", 1);
    setenv("DATABRICKS_HTTP_PATH", "/sql/1.0/warehouses/env", 1);

    databricks::Client::Config config;
    bool result = config.load_from_env();

    EXPECT_TRUE(result);
    EXPECT_EQ(config.host, "https://env.databricks.com");
    EXPECT_EQ(config.token, "env_token_789");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/env");
}

/**
 * @brief Test loading config from alternative environment variables
 */
TEST_F(ConfigTest, LoadFromEnvAlternativeNames) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);
    
    // Remove any env variables for regular names
    unsetenv("DATABRICKS_HOST");
    unsetenv("DATABRICKS_TOKEN");
    unsetenv("DATABRICKS_HTTP_PATH");

    // Set alternative names
    setenv("DATABRICKS_SERVER_HOSTNAME", "https://env.databricks.com", 1);
    setenv("DATABRICKS_ACCESS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_SQL_HTTP_PATH", "/sql/1.0/warehouses/env", 1);

    databricks::Client::Config config;
    bool result = config.load_from_env();

    EXPECT_TRUE(result);
    EXPECT_EQ(config.host, "https://env.databricks.com");
    EXPECT_EQ(config.token, "env_token");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/env");
}

/**
 * @brief Test loading config with optional timeout environment variable
 */
TEST_F(ConfigTest, LoadFromEnvWithTimeout) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);
    
    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_HTTP_PATH", "/sql/1.0/warehouses/env", 1);
    setenv("DATABRICKS_TIMEOUT", "180", 1);

    databricks::Client::Config config;
    bool result = config.load_from_env();

    EXPECT_TRUE(result);
    EXPECT_EQ(config.timeout_seconds, 180);
}

/**
 * @brief Test loading config from environment with missing variables
 */
TEST_F(ConfigTest, LoadFromEnvIncomplete) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);
    
    unsetenv("DATABRICKS_HOST");
    unsetenv("DATABRICKS_SERVER_HOSTNAME");
    setenv("DATABRICKS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_HTTP_PATH", "/sql/1.0/warehouses/env", 1);

    databricks::Client::Config config;
    bool result = config.load_from_env();

    EXPECT_FALSE(result);
}

/**
 * @brief Test from_environment prioritizes profile over env vars
 */
TEST_F(ConfigTest, FromEnvironmentPrioritizesProfile) {
    // Set up both profile and env vars
    std::string config_content = R"(
[DEFAULT]
host = https://profile.databricks.com
token = profile_token
http_path = /sql/1.0/warehouses/profile
)";
    create_config_file(config_content);

    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_HTTP_PATH", "/sql/1.0/warehouses/env", 1);

    databricks::Client::Config config = databricks::Client::Config::from_environment();

    // Should use profile values, not env vars
    EXPECT_EQ(config.host, "https://profile.databricks.com");
    EXPECT_EQ(config.token, "profile_token");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/profile");
}

/**
 * @brief Test from_environment falls back to env vars when profile missing
 */
TEST_F(ConfigTest, FromEnvironmentFallbackToEnv) {
    // No profile file, only env vars
    setenv("HOME", "/nonexistent/path", 1);
    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_HTTP_PATH", "/sql/1.0/warehouses/env", 1);

    databricks::Client::Config config = databricks::Client::Config::from_environment();

    EXPECT_EQ(config.host, "https://env.databricks.com");
    EXPECT_EQ(config.token, "env_token");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/env");
}

/**
 * @brief Test from_environment throws when no config available
 */
TEST_F(ConfigTest, FromEnvironmentThrowsWhenNoConfig) {
    // No profile file, no env vars
    setenv("HOME", "/nonexistent/path", 1);
    unsetenv("DATABRICKS_HOST");
    unsetenv("DATABRICKS_SERVER_HOSTNAME");
    unsetenv("DATABRICKS_TOKEN");
    unsetenv("DATABRICKS_ACCESS_TOKEN");
    unsetenv("DATABRICKS_HTTP_PATH");
    unsetenv("DATABRICKS_SQL_HTTP_PATH");

    EXPECT_THROW({
        databricks::Client::Config::from_environment();
    }, std::runtime_error);
}

/**
 * @brief Test from_environment with custom profile name
 */
TEST_F(ConfigTest, FromEnvironmentWithCustomProfile) {
    std::string config_content = R"(
[production]
host = https://prod.databricks.com
token = prod_token
http_path = /sql/1.0/warehouses/prod
)";
    create_config_file(config_content);

    databricks::Client::Config config = databricks::Client::Config::from_environment("production");

    EXPECT_EQ(config.host, "https://prod.databricks.com");
    EXPECT_EQ(config.token, "prod_token");
    EXPECT_EQ(config.http_path, "/sql/1.0/warehouses/prod");
}
