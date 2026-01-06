// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#include "../../mocks/mock_http_client.h"

#include <databricks/core/config.h>
#include <databricks/workspace/workspace.h>
#include <gtest/gtest.h>

using databricks::test::MockHttpClient;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

// ============================================================================
// Test Fixtures
// ============================================================================

// Constructor Tests
class WorkspaceTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        auth.timeout_seconds = 30;
    }
};

// Workspace API Mocks
class WorkspaceApiTest : public ::testing::Test {
protected:
    void SetUp() override { mock_client_ = std::make_shared<NiceMock<MockHttpClient>>(); }

    std::shared_ptr<NiceMock<MockHttpClient>> mock_client_;
};

// ============================================================================
// Constructor Tests
// ============================================================================
TEST_F(WorkspaceTest, ConstructorCreatesValidClient) {
    ASSERT_NO_THROW({databricks::Workspace workspace(auth);});
}

TEST_F(WorkspaceTest, ConstructorCreatesValidClientWithManualApiVersion) {
    const std::string& api_version = "1.0";
    ASSERT_NO_THROW({
        databricks::Workspace workspace(auth, api_version);
    });
}

TEST_F(WorkspaceTest, MultipleValidWorkspaceClient) {
    databricks::AuthConfig mockAuth;
    mockAuth.host = "https://workspace1.databricks.com";
    mockAuth.set_token("token1");

    ASSERT_NO_THROW({
        databricks::Workspace workspace1(auth);
        databricks::Workspace workspace2(mockAuth);
    });
}

// ============================================================================
// Workspace API Test
// ============================================================================

// Test: List Workspace Objects
TEST_F(WorkspaceApiTest, ListWorkspaceObjectsSuccess) {
    std::string mock_list_response = R"({
        "objects": [
            {
                "path": "/test/path/notebook1",
                "object_type": "NOTEBOOK",
                "object_id": 12345,
                "language": "PYTHON",
                "size": 1024,
                "created_at": 1609459200000,
                "modified_at": 1609545600000
            },
            {
                "path": "/test/path/directory1",
                "object_type": "DIRECTORY",
                "object_id": 67890
            }
        ]
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/list?path=/test/path"))
        .WillOnce(Return(MockHttpClient::success_response(mock_list_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/path";
    auto response = workspace.list(mock_path);

    // Verify we got 2 objects back
    ASSERT_EQ(response.size(), 2);

    // Verify first object (notebook)
    EXPECT_EQ(response[0].path, "/test/path/notebook1");
    EXPECT_EQ(response[0].object_type, databricks::ObjectType::NOTEBOOK);
    EXPECT_EQ(response[0].object_id, 12345);
    EXPECT_EQ(response[0].language, databricks::Language::PYTHON);
    EXPECT_EQ(response[0].size, 1024);
    EXPECT_EQ(response[0].created_at, 1609459200000);
    EXPECT_EQ(response[0].modified_at, 1609545600000);

    // Verify second object (directory)
    EXPECT_EQ(response[1].path, "/test/path/directory1");
    EXPECT_EQ(response[1].object_type, databricks::ObjectType::DIRECTORY);
    EXPECT_EQ(response[1].object_id, 67890);
}

// Test: List Empty Workspace Objects
TEST_F(WorkspaceApiTest, ListEmptyWorkspaceObjectsSuccess) {
    const std::string& mock_empty_list_response = R"({
        "objects": []
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/list?path=/test/path"))
        .WillOnce(Return(MockHttpClient::success_response(mock_empty_list_response)));
     
    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/path";
    auto response = workspace.list(mock_path);

    // Verify we got 0 objects back
    ASSERT_EQ(response.size(), 0);
}

// Test: Fail to list Workspace Objects with an empty path
TEST_F(WorkspaceApiTest, ListWorkspaceObjectThrowsInvalidArgument) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";

    EXPECT_THROW(workspace.list(empty_path), std::invalid_argument);
}

// Test: Create a Mock Directory
TEST_F(WorkspaceApiTest, CreateDirectorySuccess) {
    EXPECT_CALL(*mock_client_, post("/workspace/mkdirs", Eq(R"({"path":"/test/path"})")))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/path";
    workspace.mkdirs(mock_path);
}

// Test: Fail to create an empty Mock Directory
TEST_F(WorkspaceApiTest, CreateEmptyDirectoryThrowsInvalidArgument) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";

    EXPECT_THROW(workspace.mkdirs(empty_path), std::invalid_argument);
}

// Test: Get Status for a Workspace object
TEST_F(WorkspaceApiTest, GetStatusSuccess) {
    std::string mock_status_response = R"({
        "path": "/test/notebook",
        "object_type": "NOTEBOOK",
        "object_id": 12345,
        "language": "PYTHON",
        "size": 2048,
        "created_at": 1609459200000,
        "modified_at": 1609545600000
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/get-status?path=/test/notebook"))
        .WillOnce(Return(MockHttpClient::success_response(mock_status_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    auto response = workspace.get_status(mock_path);

    // Verify object status
    EXPECT_EQ(response.path, "/test/notebook");
    EXPECT_EQ(response.object_type, databricks::ObjectType::NOTEBOOK);
    EXPECT_EQ(response.object_id, 12345);
    EXPECT_EQ(response.language, databricks::Language::PYTHON);
    EXPECT_EQ(response.size, 2048);
}

// Test: Fail to get status for an empty path Workspace object
TEST_F(WorkspaceApiTest, GetStatusThrowsInvalidArgument) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";

    EXPECT_THROW(workspace.get_status(empty_path), std::invalid_argument);
}
