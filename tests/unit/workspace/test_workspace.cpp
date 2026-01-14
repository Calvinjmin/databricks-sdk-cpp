// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#include "../../mocks/mock_http_client.h"

#include <databricks/core/config.h>
#include <databricks/workspace/workspace.h>
#include <gtest/gtest.h>

using databricks::test::MockHttpClient;
using ::testing::_;
using ::testing::Eq;
using ::testing::HasSubstr;
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
    ASSERT_NO_THROW({ databricks::Workspace workspace(auth); });
}

TEST_F(WorkspaceTest, ConstructorCreatesValidClientWithManualApiVersion) {
    const std::string& api_version = "1.0";
    ASSERT_NO_THROW({ databricks::Workspace workspace(auth, api_version); });
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
// Workspace API Tests
// ============================================================================

// ----------------------------------------------------------------------------
// List Tests
// ----------------------------------------------------------------------------

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

    EXPECT_CALL(*mock_client_, get("/workspace/list?path=%2Ftest%2Fpath"))
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

    EXPECT_CALL(*mock_client_, get("/workspace/list?path=%2Ftest%2Fpath"))
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

// ----------------------------------------------------------------------------
// Mkdirs Tests
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Get Status Tests
// ----------------------------------------------------------------------------

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

    EXPECT_CALL(*mock_client_, get("/workspace/get-status?path=%2Ftest%2Fnotebook"))
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

// ============================================================================
// Export File Tests
// ============================================================================

// Test: Export file successfully with SOURCE format
TEST_F(WorkspaceApiTest, ExportFileSuccessWithSourceFormat) {
    std::string mock_export_response = R"({
        "content": "cHJpbnQoImhlbGxvIHdvcmxkIik=",
        "file_type": "py"
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/export?path=%2Ftest%2Fnotebook&format=SOURCE"))
        .WillOnce(Return(MockHttpClient::success_response(mock_export_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    auto response = workspace.export_file(mock_path, databricks::ExportFormat::SOURCE);

    // Verify the export response
    EXPECT_EQ(response.content, "cHJpbnQoImhlbGxvIHdvcmxkIik=");
    EXPECT_EQ(response.file_type, "py");
}

// Test: Export file successfully with JUPYTER format
TEST_F(WorkspaceApiTest, ExportFileSuccessWithJupyterFormat) {
    std::string mock_export_response = R"({
        "content": "eyJ2ZXJzaW9uIjogMX0=",
        "file_type": "ipynb"
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/export?path=%2Ftest%2Fnotebook&format=JUPYTER"))
        .WillOnce(Return(MockHttpClient::success_response(mock_export_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    auto response = workspace.export_file(mock_path, databricks::ExportFormat::JUPYTER);

    // Verify the export response
    EXPECT_EQ(response.content, "eyJ2ZXJzaW9uIjogMX0=");
    EXPECT_EQ(response.file_type, "ipynb");
}

// Test: Export file successfully with HTML format
TEST_F(WorkspaceApiTest, ExportFileSuccessWithHtmlFormat) {
    std::string mock_export_response = R"({
        "content": "PGh0bWw+PC9odG1sPg==",
        "file_type": "html"
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/export?path=%2Ftest%2Fnotebook&format=HTML"))
        .WillOnce(Return(MockHttpClient::success_response(mock_export_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    auto response = workspace.export_file(mock_path, databricks::ExportFormat::HTML);

    // Verify the export response
    EXPECT_EQ(response.content, "PGh0bWw+PC9odG1sPg==");
    EXPECT_EQ(response.file_type, "html");
}

// Test: Export file with DBC format for directory export
TEST_F(WorkspaceApiTest, ExportFileSuccessWithDbcFormat) {
    std::string mock_export_response = R"({
        "content": "UEsDBBQAAAAIAA==",
        "file_type": "dbc"
    })";

    EXPECT_CALL(*mock_client_, get("/workspace/export?path=%2Ftest%2Fdirectory&format=DBC"))
        .WillOnce(Return(MockHttpClient::success_response(mock_export_response)));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/directory";
    auto response = workspace.export_file(mock_path, databricks::ExportFormat::DBC);

    // Verify the export response
    EXPECT_EQ(response.content, "UEsDBBQAAAAIAA==");
    EXPECT_EQ(response.file_type, "dbc");
}

// Test: Fail to export file with empty path
TEST_F(WorkspaceApiTest, ExportFileThrowsInvalidArgument) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";

    EXPECT_THROW(workspace.export_file(empty_path, databricks::ExportFormat::SOURCE), std::invalid_argument);
}

// ============================================================================
// Import File Tests
// ============================================================================

// Test: Import file successfully with SOURCE format and Python language
TEST_F(WorkspaceApiTest, ImportFileSuccessWithSourceFormat) {
    std::string expected_body =
        R"({"content":"cHJpbnQoImhlbGxvIHdvcmxkIik=","format":"SOURCE","language":"PYTHON","overwrite":false,"path":"/test/notebook"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    const std::string& mock_content = "cHJpbnQoImhlbGxvIHdvcmxkIik=";
    workspace.import_file(mock_path, mock_content, databricks::ImportFormat::SOURCE, databricks::Language::PYTHON,
                          false);
}

// Test: Import file successfully with JUPYTER format (no language required)
TEST_F(WorkspaceApiTest, ImportFileSuccessWithJupyterFormat) {
    std::string expected_body =
        R"({"content":"eyJ2ZXJzaW9uIjogMX0=","format":"JUPYTER","overwrite":false,"path":"/test/notebook"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    const std::string& mock_content = "eyJ2ZXJzaW9uIjogMX0=";
    workspace.import_file(mock_path, mock_content, databricks::ImportFormat::JUPYTER);
}

// Test: Import file with overwrite enabled
TEST_F(WorkspaceApiTest, ImportFileSuccessWithOverwrite) {
    std::string expected_body =
        R"({"content":"cHJpbnQoImhlbGxvIHdvcmxkIik=","format":"SOURCE","language":"SCALA","overwrite":true,"path":"/test/notebook"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    const std::string& mock_content = "cHJpbnQoImhlbGxvIHdvcmxkIik=";
    workspace.import_file(mock_path, mock_content, databricks::ImportFormat::SOURCE, databricks::Language::SCALA, true);
}

// Test: Import file with AUTO format
TEST_F(WorkspaceApiTest, ImportFileSuccessWithAutoFormat) {
    std::string expected_body =
        R"({"content":"cHJpbnQoImhlbGxvIHdvcmxkIik=","format":"AUTO","overwrite":false,"path":"/test/notebook"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    const std::string& mock_content = "cHJpbnQoImhlbGxvIHdvcmxkIik=";
    workspace.import_file(mock_path, mock_content, databricks::ImportFormat::AUTO);
}

// Test: Import file with DBC format for directory import
TEST_F(WorkspaceApiTest, ImportFileSuccessWithDbcFormat) {
    std::string expected_body =
        R"({"content":"UEsDBBQAAAAIAA==","format":"DBC","overwrite":false,"path":"/test/directory"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/directory";
    const std::string& mock_content = "UEsDBBQAAAAIAA==";
    workspace.import_file(mock_path, mock_content, databricks::ImportFormat::DBC);
}

// Test: Import file using ImportRequest struct
TEST_F(WorkspaceApiTest, ImportFileSuccessWithImportRequest) {
    std::string expected_body =
        R"({"content":"cHJpbnQoImhlbGxvIHdvcmxkIik=","format":"SOURCE","language":"PYTHON","overwrite":true,"path":"/test/notebook"})";

    EXPECT_CALL(*mock_client_, post("/workspace/import", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);

    databricks::ImportRequest request;
    request.path = "/test/notebook";
    request.content = "cHJpbnQoImhlbGxvIHdvcmxkIik=";
    request.format = databricks::ImportFormat::SOURCE;
    request.language = databricks::Language::PYTHON;
    request.overwrite = true;

    workspace.import_file(request);
}

// Test: Fail to import file with empty path
TEST_F(WorkspaceApiTest, ImportFileThrowsInvalidArgumentForEmptyPath) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";
    const std::string& mock_content = "cHJpbnQoImhlbGxvIHdvcmxkIik=";

    EXPECT_THROW(workspace.import_file(empty_path, mock_content, databricks::ImportFormat::SOURCE),
                 std::invalid_argument);
}

// Test: Fail to import file with empty content
TEST_F(WorkspaceApiTest, ImportFileThrowsInvalidArgumentForEmptyContent) {
    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    const std::string& empty_content = "";

    EXPECT_THROW(workspace.import_file(mock_path, empty_content, databricks::ImportFormat::SOURCE),
                 std::invalid_argument);
}

// ============================================================================
// Delete Object Tests
// ============================================================================

// Test: Delete file successfully without recursion
TEST_F(WorkspaceApiTest, DeleteObjectSuccessNonRecursive) {
    std::string expected_body = R"({"path":"/test/notebook","recursive":false})";

    EXPECT_CALL(*mock_client_, post("/workspace/delete", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    workspace.delete_object(mock_path, false);
}

// Test: Delete directory successfully with recursion
TEST_F(WorkspaceApiTest, DeleteObjectSuccessRecursive) {
    std::string expected_body = R"({"path":"/test/directory","recursive":true})";

    EXPECT_CALL(*mock_client_, post("/workspace/delete", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/directory";
    workspace.delete_object(mock_path, true);
}

// Test: Delete object with default recursive parameter (false)
TEST_F(WorkspaceApiTest, DeleteObjectSuccessDefaultRecursive) {
    std::string expected_body = R"({"path":"/test/notebook","recursive":false})";

    EXPECT_CALL(*mock_client_, post("/workspace/delete", Eq(expected_body)))
        .WillOnce(Return(MockHttpClient::success_response("")));

    databricks::Workspace workspace(mock_client_);
    const std::string& mock_path = "/test/notebook";
    workspace.delete_object(mock_path); // Default recursive=false
}

// Test: Fail to delete object with empty path
TEST_F(WorkspaceApiTest, DeleteObjectThrowsInvalidArgument) {
    databricks::Workspace workspace(mock_client_);
    const std::string& empty_path = "";

    EXPECT_THROW(workspace.delete_object(empty_path, false), std::invalid_argument);
}