// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "databricks/core/config.h"

#include <cstdlib>
#include <fstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace databricks;
using ::testing::HasSubstr;
using ::testing::Not;

// =============================================================================
// AuthConfig Error Handling Tests
// =============================================================================

class AuthConfigErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear all relevant environment variables
        unsetenv("DATABRICKS_HOST");
        unsetenv("DATABRICKS_SERVER_HOSTNAME");
        unsetenv("DATABRICKS_TOKEN");
        unsetenv("DATABRICKS_ACCESS_TOKEN");
        unsetenv("DATABRICKS_TIMEOUT");
        unsetenv("HOME");
    }

    void TearDown() override {
        // Clean up any test files created
        // Restore environment if needed
    }
};

// Test that from_environment provides detailed error when both profile and env fail
TEST_F(AuthConfigErrorTest, FromEnvironmentDetailedError) {
    // Set HOME but don't create .databrickscfg file
    setenv("HOME", "/tmp/nonexistent_home_test_error_handling", 1);

    try {
        AuthConfig::from_environment("DEFAULT");
        FAIL() << "Expected std::runtime_error to be thrown";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should contain the general error message
        EXPECT_THAT(error_msg, HasSubstr("Failed to load Databricks authentication configuration"));

        // Should contain "Detailed errors:" section
        EXPECT_THAT(error_msg, HasSubstr("Detailed errors:"));

        // Should mention both failures (profile and environment)
        EXPECT_THAT(error_msg, HasSubstr("1."));
        EXPECT_THAT(error_msg, HasSubstr("2."));

        // Should have helpful instructions
        EXPECT_THAT(error_msg, HasSubstr("~/.databrickscfg"));
        EXPECT_THAT(error_msg, HasSubstr("DATABRICKS_HOST"));
        EXPECT_THAT(error_msg, HasSubstr("DATABRICKS_TOKEN"));
    }
}

// Test that profile-only failure is logged
TEST_F(AuthConfigErrorTest, ProfileFailureLogged) {
    setenv("HOME", "/tmp/nonexistent_home_test_profile", 1);

    try {
        AuthConfig::from_profile("DEFAULT");
        FAIL() << "Expected std::runtime_error to be thrown";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        EXPECT_THAT(error_msg, HasSubstr("Could not open ~/.databrickscfg"));
    }
}

// Test that environment-only failure provides specific error
TEST_F(AuthConfigErrorTest, EnvironmentFailureSpecific) {
    try {
        AuthConfig::from_env();
        FAIL() << "Expected std::runtime_error to be thrown";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should mention which environment variables are missing
        EXPECT_THAT(error_msg, HasSubstr("DATABRICKS_HOST"));
        EXPECT_THAT(error_msg, HasSubstr("DATABRICKS_SERVER_HOSTNAME"));
    }
}

// Test that successful profile loading works (no regression)
TEST_F(AuthConfigErrorTest, SuccessfulProfileLoading) {
    // Create a temporary directory and .databrickscfg file
    const char* home = "/tmp/test_databricks_config_success";
    setenv("HOME", home, 1);

    // Create the directory if it doesn't exist
    system(("mkdir -p " + std::string(home)).c_str());

    // Create the config file
    std::string config_path = std::string(home) + "/.databrickscfg";
    std::ofstream config_file(config_path);
    ASSERT_TRUE(config_file.is_open()) << "Failed to create test config file at " << config_path;

    config_file << "[DEFAULT]\n";
    config_file << "host = https://test.databricks.com\n";
    config_file << "token = test_token_12345\n";
    config_file.close();

    // Should load successfully
    AuthConfig config = AuthConfig::from_profile("DEFAULT");

    EXPECT_EQ(config.host, "https://test.databricks.com");
    EXPECT_TRUE(config.has_secure_token());

    // Clean up
    std::remove(config_path.c_str());
}

// Test that successful environment loading works (no regression)
TEST_F(AuthConfigErrorTest, SuccessfulEnvironmentLoading) {
    setenv("DATABRICKS_HOST", "https://env-test.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "env_token_67890", 1);

    AuthConfig config = AuthConfig::from_env();

    EXPECT_EQ(config.host, "https://env-test.databricks.com");
    EXPECT_TRUE(config.has_secure_token());
}

// Test fallback behavior: profile fails, env succeeds
TEST_F(AuthConfigErrorTest, FallbackToEnvironment) {
    // No profile file exists
    setenv("HOME", "/tmp/nonexistent_for_fallback", 1);

    // But environment variables are set
    setenv("DATABRICKS_HOST", "https://fallback.databricks.com", 1);
    setenv("DATABRICKS_TOKEN", "fallback_token", 1);

    // Should fall back to environment and succeed
    AuthConfig config = AuthConfig::from_environment("DEFAULT");

    EXPECT_EQ(config.host, "https://fallback.databricks.com");
    EXPECT_TRUE(config.has_secure_token());
}

// Test that profile with missing required fields provides clear error
TEST_F(AuthConfigErrorTest, ProfileMissingFields) {
    const char* home = "/tmp/test_databricks_missing_fields";
    setenv("HOME", home, 1);

    // Create the directory if it doesn't exist
    system(("mkdir -p " + std::string(home)).c_str());

    // Create config file with only host (missing token)
    std::string config_path = std::string(home) + "/.databrickscfg";
    std::ofstream config_file(config_path);
    ASSERT_TRUE(config_file.is_open()) << "Failed to create test config file at " << config_path;

    config_file << "[DEFAULT]\n";
    config_file << "host = https://incomplete.databricks.com\n";
    config_file.close();

    try {
        AuthConfig::from_profile("DEFAULT");
        FAIL() << "Expected std::runtime_error for missing token";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        EXPECT_THAT(error_msg, HasSubstr("missing required fields"));
        EXPECT_THAT(error_msg, HasSubstr("host"));
        EXPECT_THAT(error_msg, HasSubstr("token"));
    }

    // Clean up
    std::remove(config_path.c_str());
}

// Test that from_environment aggregates errors from both sources
TEST_F(AuthConfigErrorTest, AggregatesMultipleErrors) {
    setenv("HOME", "/tmp/test_aggregate_errors", 1);
    // No config file, no env vars

    try {
        AuthConfig::from_environment("CUSTOM_PROFILE");
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should mention CUSTOM_PROFILE specifically
        EXPECT_THAT(error_msg, HasSubstr("CUSTOM_PROFILE"));

        // Should have numbered error list
        EXPECT_THAT(error_msg, HasSubstr("1."));
        EXPECT_THAT(error_msg, HasSubstr("2."));
    }
}

// Note: main() is provided by gtest_main linked in CMakeLists.txt
