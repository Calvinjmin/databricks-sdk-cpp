#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/core/config.h>
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
 * @brief Test AuthConfig default construction
 */
TEST_F(ConfigTest, AuthConfigDefaultConstruction) {
    databricks::AuthConfig auth;
    EXPECT_TRUE(auth.host.empty());
    EXPECT_FALSE(auth.has_secure_token());
    EXPECT_EQ(auth.timeout_seconds, 60);
}

/**
 * @brief Test SQLConfig default construction
 */
TEST_F(ConfigTest, SQLConfigDefaultConstruction) {
    databricks::SQLConfig sql;
    EXPECT_TRUE(sql.http_path.empty());
    EXPECT_EQ(sql.odbc_driver_name, "Simba Spark ODBC Driver");
}

/**
 * @brief Test PoolingConfig default construction
 */
TEST_F(ConfigTest, PoolingConfigDefaultConstruction) {
    databricks::PoolingConfig pooling;
    EXPECT_FALSE(pooling.enabled);
    EXPECT_EQ(pooling.min_connections, 1);
    EXPECT_EQ(pooling.max_connections, 10);
    EXPECT_EQ(pooling.connection_timeout_ms, 5000);
}

/**
 * @brief Test loading AuthConfig from profile with DEFAULT section
 */
TEST_F(ConfigTest, LoadAuthConfigFromProfile) {
    std::string config_content = R"(
[DEFAULT]
host = https://profile.databricks.com
token = profile_token_123
)";
    create_config_file(config_content);

    databricks::AuthConfig auth = databricks::AuthConfig::from_profile("DEFAULT");

    EXPECT_EQ(auth.host, "https://profile.databricks.com");
    EXPECT_TRUE(auth.has_secure_token());
}

/**
 * @brief Test loading AuthConfig from named profile
 */
TEST_F(ConfigTest, LoadAuthConfigFromNamedProfile) {
    std::string config_content = R"(
[DEFAULT]
host = https://default.databricks.com
token = default_token

[production]
host = https://production.databricks.com
token = prod_token_456

[staging]
host = https://staging.databricks.com
token = staging_token
)";
    create_config_file(config_content);

    databricks::AuthConfig auth = databricks::AuthConfig::from_profile("production");

    EXPECT_EQ(auth.host, "https://production.databricks.com");
    EXPECT_TRUE(auth.has_secure_token());
}

/**
 * @brief Test loading AuthConfig from environment variables
 */
TEST_F(ConfigTest, LoadAuthConfigFromEnv) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);

    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token_789", 1);

    databricks::AuthConfig auth = databricks::AuthConfig::from_env();

    EXPECT_EQ(auth.host, "https://env.databricks.com");
    EXPECT_TRUE(auth.has_secure_token());
}

/**
 * @brief Test loading AuthConfig from environment with timeout
 */
TEST_F(ConfigTest, LoadAuthConfigFromEnvWithTimeout) {
    // Point HOME to temp directory to avoid loading real ~/.databrickscfg
    setenv("HOME", temp_dir.c_str(), 1);

    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);
    setenv("DATABRICKS_TIMEOUT", "180", 1);

    databricks::AuthConfig auth = databricks::AuthConfig::from_env();

    EXPECT_EQ(auth.timeout_seconds, 180);
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
)";
    create_config_file(config_content);

    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);

    databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

    // Should use profile values, not env vars
    EXPECT_EQ(auth.host, "https://profile.databricks.com");
    EXPECT_TRUE(auth.has_secure_token());
}

/**
 * @brief Test from_environment falls back to env vars when profile missing
 */
TEST_F(ConfigTest, FromEnvironmentFallbackToEnv) {
    // No profile file, only env vars
    setenv("HOME", "/nonexistent/path", 1);
    setenv("DATABRICKS_HOST", "https://env.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token", 1);

    databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

    EXPECT_EQ(auth.host, "https://env.databricks.com");
    EXPECT_TRUE(auth.has_secure_token());
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

    EXPECT_THROW({
        databricks::AuthConfig::from_environment();
    }, std::runtime_error);
}

/**
 * @brief Test AuthConfig validation
 */
TEST_F(ConfigTest, AuthConfigValidation) {
    databricks::AuthConfig auth;

    // Empty config is invalid
    EXPECT_FALSE(auth.is_valid());

    // Host only is invalid
    auth.host = "https://test.databricks.com";
    EXPECT_FALSE(auth.is_valid());

    // Host + token is valid
    auth.set_token("test_token");
    EXPECT_TRUE(auth.is_valid());

    // Zero timeout makes it invalid
    auth.timeout_seconds = 0;
    EXPECT_FALSE(auth.is_valid());
}

/**
 * @brief Test SQLConfig validation
 */
TEST_F(ConfigTest, SQLConfigValidation) {
    databricks::SQLConfig sql;

    // Empty http_path is invalid
    EXPECT_FALSE(sql.is_valid());

    // With http_path is valid
    sql.http_path = "/sql/1.0/warehouses/test";
    EXPECT_TRUE(sql.is_valid());
}

/**
 * @brief Test PoolingConfig validation
 */
TEST_F(ConfigTest, PoolingConfigValidation) {
    databricks::PoolingConfig pooling;

    // Default config is valid
    EXPECT_TRUE(pooling.is_valid());

    // Zero min_connections is invalid
    pooling.min_connections = 0;
    EXPECT_FALSE(pooling.is_valid());
    pooling.min_connections = 1;

    // max < min is invalid
    pooling.min_connections = 10;
    pooling.max_connections = 5;
    EXPECT_FALSE(pooling.is_valid());
    pooling.max_connections = 10;

    // Zero timeout is invalid
    pooling.connection_timeout_ms = 0;
    EXPECT_FALSE(pooling.is_valid());
}

