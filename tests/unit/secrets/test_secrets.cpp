// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "../../mocks/mock_http_client.h"

#include <databricks/core/config.h>
#include <databricks/secrets/secrets.h>
#include <gtest/gtest.h>

using databricks::test::MockHttpClient;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;

// ============================================================================
// Test Fixtures
// ============================================================================

// Test fixture for Secrets constructor tests
class SecretsTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        auth.timeout_seconds = 30;
    }
};

// Test fixture for Secrets API tests with mocks
class SecretsApiTest : public ::testing::Test {
protected:
    void SetUp() override { mock_client_ = std::make_shared<NiceMock<MockHttpClient>>(); }

    std::shared_ptr<NiceMock<MockHttpClient>> mock_client_;
};

// ============================================================================
// Constructor Tests
// ============================================================================

// Test: Secrets client construction
TEST_F(SecretsTest, ConstructorCreatesValidClient) {
    ASSERT_NO_THROW({ databricks::Secrets secrets(auth); });
}

// Test: Secrets client can be constructed with minimal config
TEST_F(SecretsTest, MinimalConfigConstruction) {
    databricks::AuthConfig minimal_auth;
    minimal_auth.host = "https://minimal.databricks.com";
    minimal_auth.set_token("token");

    ASSERT_NO_THROW({ databricks::Secrets secrets(minimal_auth); });
}

// Test: Multiple Secrets clients can coexist
TEST_F(SecretsTest, MultipleClientsCanCoexist) {
    databricks::AuthConfig auth1;
    auth1.host = "https://workspace1.databricks.com";
    auth1.set_token("token1");

    databricks::AuthConfig auth2;
    auth2.host = "https://workspace2.databricks.com";
    auth2.set_token("token2");

    ASSERT_NO_THROW({
        databricks::Secrets secrets1(auth1);
        databricks::Secrets secrets2(auth2);
        // Both should coexist without issues
    });
}

// ============================================================================
// Struct Tests - SecretScope
// ============================================================================

// Test: SecretScope struct initialization and default values
TEST(SecretScopeStructTest, DefaultInitialization) {
    databricks::SecretScope scope;

    EXPECT_EQ(scope.name, "");
    EXPECT_EQ(scope.backend_type, databricks::SecretScopeBackendType::DATABRICKS);
    EXPECT_EQ(scope.resource_id, "");
    EXPECT_EQ(scope.dns_name, "");
}

// Test: SecretScope from valid DATABRICKS JSON
TEST(SecretScopeStructTest, ParseValidDatabricksJson) {
    std::string json = R"({
        "name": "my-scope",
        "backend_type": "DATABRICKS"
    })";

    databricks::SecretScope scope = databricks::SecretScope::from_json(json);

    EXPECT_EQ(scope.name, "my-scope");
    EXPECT_EQ(scope.backend_type, databricks::SecretScopeBackendType::DATABRICKS);
}

// Test: SecretScope from valid AZURE_KEYVAULT JSON
TEST(SecretScopeStructTest, ParseValidAzureKeyVaultJson) {
    std::string json = R"({
        "name": "azure-scope",
        "backend_type": "AZURE_KEYVAULT",
        "keyvault_metadata": {
            "resource_id": "/subscriptions/abc/resourceGroups/rg/providers/Microsoft.KeyVault/vaults/vault",
            "dns_name": "https://vault.vault.azure.net/"
        }
    })";

    databricks::SecretScope scope = databricks::SecretScope::from_json(json);

    EXPECT_EQ(scope.name, "azure-scope");
    EXPECT_EQ(scope.backend_type, databricks::SecretScopeBackendType::AZURE_KEYVAULT);
    EXPECT_EQ(scope.resource_id, "/subscriptions/abc/resourceGroups/rg/providers/Microsoft.KeyVault/vaults/vault");
    EXPECT_EQ(scope.dns_name, "https://vault.vault.azure.net/");
}

// Test: SecretScope handles unknown backend type
TEST(SecretScopeStructTest, HandlesUnknownBackendType) {
    std::string json = R"({
        "name": "unknown-scope",
        "backend_type": "UNKNOWN_TYPE"
    })";

    databricks::SecretScope scope = databricks::SecretScope::from_json(json);

    EXPECT_EQ(scope.name, "unknown-scope");
    EXPECT_EQ(scope.backend_type, databricks::SecretScopeBackendType::UNKNOWN);
}

// Test: SecretScope rejects invalid JSON
TEST(SecretScopeStructTest, RejectsInvalidJson) {
    std::string invalid_json = "not valid json";

    EXPECT_THROW({ databricks::SecretScope::from_json(invalid_json); }, std::runtime_error);
}

// ============================================================================
// Struct Tests - Secret
// ============================================================================

// Test: Secret struct initialization and default values
TEST(SecretStructTest, DefaultInitialization) {
    databricks::Secret secret;

    EXPECT_EQ(secret.key, "");
    EXPECT_EQ(secret.last_updated_timestamp, 0);
}

// Test: Secret from valid JSON
TEST(SecretStructTest, ParseValidJson) {
    std::string json = R"({
        "key": "api-key",
        "last_updated_timestamp": 1609459200000
    })";

    databricks::Secret secret = databricks::Secret::from_json(json);

    EXPECT_EQ(secret.key, "api-key");
    EXPECT_EQ(secret.last_updated_timestamp, 1609459200000);
}

// Test: Secret handles missing timestamp
TEST(SecretStructTest, HandlesMissingTimestamp) {
    std::string json = R"({
        "key": "test-key"
    })";

    ASSERT_NO_THROW({
        databricks::Secret secret = databricks::Secret::from_json(json);
        EXPECT_EQ(secret.key, "test-key");
        EXPECT_EQ(secret.last_updated_timestamp, 0);
    });
}

// Test: Secret rejects invalid JSON
TEST(SecretStructTest, RejectsInvalidJson) {
    std::string invalid_json = "invalid json";

    EXPECT_THROW({ databricks::Secret::from_json(invalid_json); }, std::runtime_error);
}

// ============================================================================
// Struct Tests - SecretACL
// ============================================================================

// Test: SecretACL struct initialization and default values
TEST(SecretACLStructTest, DefaultInitialization) {
    databricks::SecretACL acl;

    EXPECT_EQ(acl.principal, "");
    EXPECT_EQ(acl.permission, databricks::SecretPermission::READ);
}

// Test: SecretACL from valid JSON with READ permission
TEST(SecretACLStructTest, ParseValidJsonRead) {
    std::string json = R"({
        "principal": "users",
        "permission": "READ"
    })";

    databricks::SecretACL acl = databricks::SecretACL::from_json(json);

    EXPECT_EQ(acl.principal, "users");
    EXPECT_EQ(acl.permission, databricks::SecretPermission::READ);
}

// Test: SecretACL from valid JSON with WRITE permission
TEST(SecretACLStructTest, ParseValidJsonWrite) {
    std::string json = R"({
        "principal": "admins",
        "permission": "WRITE"
    })";

    databricks::SecretACL acl = databricks::SecretACL::from_json(json);

    EXPECT_EQ(acl.principal, "admins");
    EXPECT_EQ(acl.permission, databricks::SecretPermission::WRITE);
}

// Test: SecretACL from valid JSON with MANAGE permission
TEST(SecretACLStructTest, ParseValidJsonManage) {
    std::string json = R"({
        "principal": "user@example.com",
        "permission": "MANAGE"
    })";

    databricks::SecretACL acl = databricks::SecretACL::from_json(json);

    EXPECT_EQ(acl.principal, "user@example.com");
    EXPECT_EQ(acl.permission, databricks::SecretPermission::MANAGE);
}

// Test: SecretACL handles unknown permission
TEST(SecretACLStructTest, HandlesUnknownPermission) {
    std::string json = R"({
        "principal": "users",
        "permission": "UNKNOWN_PERMISSION"
    })";

    databricks::SecretACL acl = databricks::SecretACL::from_json(json);

    EXPECT_EQ(acl.principal, "users");
    EXPECT_EQ(acl.permission, databricks::SecretPermission::UNKNOWN);
}

// Test: SecretACL rejects invalid JSON
TEST(SecretACLStructTest, RejectsInvalidJson) {
    std::string invalid_json = "invalid json";

    EXPECT_THROW({ databricks::SecretACL::from_json(invalid_json); }, std::runtime_error);
}

// ============================================================================
// API Method Tests - list_scopes
// ============================================================================

// Test: List scopes with valid response
TEST_F(SecretsApiTest, ListScopesValid) {
    std::string response_body = R"({
        "scopes": [
            {
                "name": "test-scope-1",
                "backend_type": "DATABRICKS"
            },
            {
                "name": "test-scope-2",
                "backend_type": "AZURE_KEYVAULT",
                "keyvault_metadata": {
                    "resource_id": "/subscriptions/test/resourceGroups/test/providers/Microsoft.KeyVault/vaults/test",
                    "dns_name": "https://test.vault.azure.net/"
                }
            }
        ]
    })";

    EXPECT_CALL(*mock_client_, get("/secrets/scopes/list"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto scopes = secrets.list_scopes();

    ASSERT_EQ(scopes.size(), 2);

    // Check first scope (DATABRICKS backend)
    EXPECT_EQ(scopes[0].name, "test-scope-1");
    EXPECT_EQ(scopes[0].backend_type, databricks::SecretScopeBackendType::DATABRICKS);

    // Check second scope (AZURE_KEYVAULT backend)
    EXPECT_EQ(scopes[1].name, "test-scope-2");
    EXPECT_EQ(scopes[1].backend_type, databricks::SecretScopeBackendType::AZURE_KEYVAULT);
    EXPECT_EQ(scopes[1].resource_id,
              "/subscriptions/test/resourceGroups/test/providers/Microsoft.KeyVault/vaults/test");
    EXPECT_EQ(scopes[1].dns_name, "https://test.vault.azure.net/");
}

// Test: List scopes with empty response
TEST_F(SecretsApiTest, ListScopesEmpty) {
    std::string response_body = R"({"scopes": []})";

    EXPECT_CALL(*mock_client_, get("/secrets/scopes/list"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto scopes = secrets.list_scopes();

    EXPECT_EQ(scopes.size(), 0);
}

// Test: List scopes with no scopes field
TEST_F(SecretsApiTest, ListScopesNoScopesField) {
    std::string response_body = R"({"other_field": "value"})";

    EXPECT_CALL(*mock_client_, get("/secrets/scopes/list"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto scopes = secrets.list_scopes();

    EXPECT_EQ(scopes.size(), 0);
}

// ============================================================================
// API Method Tests - create_scope
// ============================================================================

// Test: Create Databricks-backed scope successfully
TEST_F(SecretsApiTest, CreateDatabricksScope) {
    EXPECT_CALL(*mock_client_, post("/secrets/scopes/create", HasSubstr("\"scope\":\"test-scope\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.create_scope("test-scope", "users", databricks::SecretScopeBackendType::DATABRICKS); });
}

// Test: Create scope with custom initial_manage_principal
TEST_F(SecretsApiTest, CreateScopeWithCustomPrincipal) {
    EXPECT_CALL(*mock_client_, post("/secrets/scopes/create", HasSubstr("\"initial_manage_principal\":\"admins\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.create_scope("admin-scope", "admins", databricks::SecretScopeBackendType::DATABRICKS); });
}

// Test: Create Azure Key Vault-backed scope successfully
TEST_F(SecretsApiTest, CreateAzureKeyVaultScope) {
    EXPECT_CALL(*mock_client_, post("/secrets/scopes/create", HasSubstr("AZURE_KEYVAULT")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({
        secrets.create_scope("azure-scope", "users", databricks::SecretScopeBackendType::AZURE_KEYVAULT,
                             "/subscriptions/abc/resourceGroups/rg/providers/Microsoft.KeyVault/vaults/vault",
                             "tenant-id-123", "https://vault.vault.azure.net/");
    });
}

// Test: Create Azure Key Vault scope without required parameters throws
TEST_F(SecretsApiTest, CreateAzureKeyVaultScopeThrowsWithoutRequiredParams) {
    databricks::Secrets secrets(mock_client_);

    // Missing all Azure parameters
    EXPECT_THROW(
        { secrets.create_scope("azure-scope", "users", databricks::SecretScopeBackendType::AZURE_KEYVAULT); },
        std::invalid_argument);

    // Missing tenant_id
    EXPECT_THROW(
        {
            secrets.create_scope("azure-scope", "users", databricks::SecretScopeBackendType::AZURE_KEYVAULT,
                                 "/subscriptions/abc/resourceGroups/rg/providers/Microsoft.KeyVault/vaults/vault");
        },
        std::invalid_argument);
}

// ============================================================================
// API Method Tests - delete_scope
// ============================================================================

// Test: Delete scope successfully
TEST_F(SecretsApiTest, DeleteScopeSuccess) {
    EXPECT_CALL(*mock_client_, post("/secrets/scopes/delete", HasSubstr("\"scope\":\"test-scope\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.delete_scope("test-scope"); });
}

// ============================================================================
// API Method Tests - put_secret
// ============================================================================

// Test: Put secret successfully
TEST_F(SecretsApiTest, PutSecretSuccess) {
    EXPECT_CALL(*mock_client_, post("/secrets/put", HasSubstr("\"scope\":\"test-scope\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.put_secret("test-scope", "api-key", "secret-value-123"); });
}

// Test: Put secret with empty value
TEST_F(SecretsApiTest, PutSecretEmptyValue) {
    EXPECT_CALL(*mock_client_, post("/secrets/put", HasSubstr("\"string_value\":\"\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.put_secret("test-scope", "empty-key", ""); });
}

// ============================================================================
// API Method Tests - delete_secret
// ============================================================================

// Test: Delete secret successfully
TEST_F(SecretsApiTest, DeleteSecretSuccess) {
    EXPECT_CALL(*mock_client_, post("/secrets/delete", HasSubstr("\"scope\":\"test-scope\"")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Secrets secrets(mock_client_);
    ASSERT_NO_THROW({ secrets.delete_secret("test-scope", "api-key"); });
}

// ============================================================================
// API Method Tests - list_secrets
// ============================================================================

// Test: List secrets with valid response
TEST_F(SecretsApiTest, ListSecretsValid) {
    std::string response_body = R"({
        "secrets": [
            {
                "key": "secret-1",
                "last_updated_timestamp": 1609459200000
            },
            {
                "key": "secret-2",
                "last_updated_timestamp": 1609462800000
            }
        ]
    })";

    EXPECT_CALL(*mock_client_, get("/secrets/list?scope=test-scope"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto secret_list = secrets.list_secrets("test-scope");

    ASSERT_EQ(secret_list.size(), 2);
    EXPECT_EQ(secret_list[0].key, "secret-1");
    EXPECT_EQ(secret_list[0].last_updated_timestamp, 1609459200000);
    EXPECT_EQ(secret_list[1].key, "secret-2");
    EXPECT_EQ(secret_list[1].last_updated_timestamp, 1609462800000);
}

// Test: List secrets with empty response
TEST_F(SecretsApiTest, ListSecretsEmpty) {
    std::string response_body = R"({"secrets": []})";

    EXPECT_CALL(*mock_client_, get("/secrets/list?scope=empty-scope"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto secret_list = secrets.list_secrets("empty-scope");

    EXPECT_EQ(secret_list.size(), 0);
}

// Test: List secrets with no secrets field
TEST_F(SecretsApiTest, ListSecretsNoSecretsField) {
    std::string response_body = R"({"other_field": "value"})";

    EXPECT_CALL(*mock_client_, get("/secrets/list?scope=test-scope"))
        .WillOnce(Return(MockHttpClient::success_response(response_body)));

    databricks::Secrets secrets(mock_client_);
    auto secret_list = secrets.list_secrets("test-scope");

    EXPECT_EQ(secret_list.size(), 0);
}
