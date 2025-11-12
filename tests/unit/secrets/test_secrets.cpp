// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "../../mocks/mock_http_client.h"

#include <databricks/core/config.h>
#include <databricks/secrets/secrets.h>
#include <gtest/gtest.h>

using databricks::test::MockHttpClient;
using ::testing::_;
using ::testing::Return;

// Internal Fixtures for Secrets Test
class SecretsTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        auth.timeout_seconds = 30;
    }
};

// Internal Mock HTTP Fixture for Secrets API
class SecretsApiTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        auth.timeout_seconds = 30;
    }
};

// Test: Secrets constructor
TEST_F(SecretsTest, ConstructorCreatesValidClient) {
    ASSERT_NO_THROW({ databricks::Secrets secrets(auth); });
}