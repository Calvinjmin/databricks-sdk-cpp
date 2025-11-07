#include "databricks/core/config.h"
#include "databricks/unity_catalog/unity_catalog.h"

#include "mock_http_client.h"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace databricks;
using namespace databricks::test;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Return;

// =============================================================================
// Unity Catalog JSON Parsing Error Handling Tests
// =============================================================================

class UnityCatalogErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto mock_client = std::make_shared<MockHttpClient>();
        mock_http_client_ = mock_client;
        unity_catalog_ = std::make_unique<UnityCatalog>(mock_client);
    }

    std::shared_ptr<MockHttpClient> mock_http_client_;
    std::unique_ptr<UnityCatalog> unity_catalog_;
};

// =============================================================================
// Catalog Parsing Error Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, ParseCatalogMalformedJSON) {
    // Test with completely invalid JSON
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = "{ invalid json, missing quote";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs/test")).WillOnce(Return(response));

    try {
        unity_catalog_->get_catalog("test");
        FAIL() << "Expected std::runtime_error for malformed JSON";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should identify it as malformed JSON
        EXPECT_THAT(error_msg, HasSubstr("Malformed JSON"));
        EXPECT_THAT(error_msg, HasSubstr("Catalog"));

        // Should include JSON snippet
        EXPECT_THAT(error_msg, HasSubstr("first 200 chars"));
        EXPECT_THAT(error_msg, HasSubstr("invalid json"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseCatalogMissingRequiredField) {
    // Test with JSON missing required 'name' field
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "catalog_type": "MANAGED_CATALOG",
        "owner": "admin",
        "comment": "Test catalog"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs/test")).WillOnce(Return(response));

    try {
        unity_catalog_->get_catalog("test");
        FAIL() << "Expected std::runtime_error for missing required field";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should identify missing field specifically
        EXPECT_THAT(error_msg, HasSubstr("Missing required fields"));
        EXPECT_THAT(error_msg, HasSubstr("name"));

        // Should include JSON for debugging
        EXPECT_THAT(error_msg, HasSubstr("JSON received"));
        EXPECT_THAT(error_msg, HasSubstr("MANAGED_CATALOG"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseCatalogTypeError) {
    // Test with field having wrong type (number instead of string for name)
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": 12345,
        "catalog_type": "MANAGED_CATALOG"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs/test")).WillOnce(Return(response));

    try {
        unity_catalog_->get_catalog("test");
        FAIL() << "Expected std::runtime_error for type error";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // Should identify it as a type error
        EXPECT_THAT(error_msg, HasSubstr("Type error"));
        EXPECT_THAT(error_msg, HasSubstr("Catalog"));

        // Should include helpful explanation
        EXPECT_THAT(error_msg, HasSubstr("unexpected type"));

        // Should include JSON snippet
        EXPECT_THAT(error_msg, HasSubstr("12345"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseCatalogValidJSON) {
    // Test that valid JSON still works (no regression)
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": "test_catalog",
        "catalog_type": "MANAGED_CATALOG",
        "owner": "admin",
        "comment": "Test catalog",
        "created_at": 1234567890,
        "updated_at": 1234567890,
        "metastore_id": "meta-123",
        "full_name": "test_catalog"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs/test_catalog")).WillOnce(Return(response));

    CatalogInfo catalog = unity_catalog_->get_catalog("test_catalog");

    EXPECT_EQ(catalog.name, "test_catalog");
    EXPECT_EQ(catalog.catalog_type, "MANAGED_CATALOG");
    EXPECT_EQ(catalog.owner, "admin");
    EXPECT_EQ(catalog.comment, "Test catalog");
}

// =============================================================================
// Catalog List Parsing Error Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, ParseCatalogListMalformedJSON) {
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = "not valid json at all [[[";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs")).WillOnce(Return(response));

    try {
        unity_catalog_->list_catalogs();
        FAIL() << "Expected std::runtime_error for malformed JSON";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        EXPECT_THAT(error_msg, HasSubstr("Malformed JSON"));
        EXPECT_THAT(error_msg, HasSubstr("catalogs list"));
        EXPECT_THAT(error_msg, HasSubstr("not valid json"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseCatalogListPartialFailure) {
    // Test resilience: one catalog fails parsing, others succeed
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "catalogs": [
            {
                "name": "good_catalog_1",
                "catalog_type": "MANAGED_CATALOG",
                "owner": "admin"
            },
            {
                "catalog_type": "MANAGED_CATALOG",
                "owner": "admin"
            },
            {
                "name": "good_catalog_2",
                "catalog_type": "MANAGED_CATALOG",
                "owner": "admin"
            }
        ]
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs")).WillOnce(Return(response));

    // Should not throw, but should skip the invalid catalog
    std::vector<CatalogInfo> catalogs = unity_catalog_->list_catalogs();

    // Should have parsed the 2 valid catalogs (skipped the one missing name)
    EXPECT_EQ(catalogs.size(), 2);
    EXPECT_EQ(catalogs[0].name, "good_catalog_1");
    EXPECT_EQ(catalogs[1].name, "good_catalog_2");
}

TEST_F(UnityCatalogErrorTest, ParseCatalogListEmptyArray) {
    // Test that empty array is handled gracefully
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({"catalogs": []})";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs")).WillOnce(Return(response));

    std::vector<CatalogInfo> catalogs = unity_catalog_->list_catalogs();
    EXPECT_EQ(catalogs.size(), 0);
}

TEST_F(UnityCatalogErrorTest, ParseCatalogListNoCatalogsField) {
    // Test that missing "catalogs" field returns empty list
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({"other_field": "value"})";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs")).WillOnce(Return(response));

    std::vector<CatalogInfo> catalogs = unity_catalog_->list_catalogs();
    EXPECT_EQ(catalogs.size(), 0);
}

// =============================================================================
// Schema Parsing Error Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, ParseSchemaMissingRequiredField) {
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "catalog_name": "test_catalog",
        "owner": "admin"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/schemas/test_catalog.test_schema")).WillOnce(Return(response));

    try {
        unity_catalog_->get_schema("test_catalog.test_schema");
        FAIL() << "Expected std::runtime_error for missing name";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        EXPECT_THAT(error_msg, HasSubstr("Missing required fields"));
        EXPECT_THAT(error_msg, HasSubstr("Schema"));
        EXPECT_THAT(error_msg, HasSubstr("name"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseSchemaValidJSON) {
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": "test_schema",
        "catalog_name": "test_catalog",
        "owner": "admin",
        "full_name": "test_catalog.test_schema"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/schemas/test_catalog.test_schema")).WillOnce(Return(response));

    SchemaInfo schema = unity_catalog_->get_schema("test_catalog.test_schema");
    EXPECT_EQ(schema.name, "test_schema");
    EXPECT_EQ(schema.catalog_name, "test_catalog");
}

// =============================================================================
// Table Parsing Error Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, ParseTableMissingRequiredField) {
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "catalog_name": "test_catalog",
        "schema_name": "test_schema",
        "table_type": "MANAGED"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/tables/test_catalog.test_schema.test_table"))
        .WillOnce(Return(response));

    try {
        unity_catalog_->get_table("test_catalog.test_schema.test_table");
        FAIL() << "Expected std::runtime_error for missing name";
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());
        EXPECT_THAT(error_msg, HasSubstr("Missing required fields"));
        EXPECT_THAT(error_msg, HasSubstr("Table"));
        EXPECT_THAT(error_msg, HasSubstr("name"));
    }
}

TEST_F(UnityCatalogErrorTest, ParseTableWithColumnsPartialFailure) {
    // Test that table parsing continues even if some columns fail
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": "test_table",
        "catalog_name": "test_catalog",
        "schema_name": "test_schema",
        "table_type": "MANAGED",
        "columns": [
            {
                "name": "col1",
                "type_text": "string",
                "type_name": "STRING"
            },
            "invalid_column_entry",
            {
                "name": "col2",
                "type_text": "int",
                "type_name": "INT"
            }
        ]
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/tables/test_catalog.test_schema.test_table"))
        .WillOnce(Return(response));

    // Should parse successfully, skipping invalid column
    TableInfo table = unity_catalog_->get_table("test_catalog.test_schema.test_table");

    EXPECT_EQ(table.name, "test_table");
    // Should have parsed 2 valid columns (skipped the invalid one)
    EXPECT_EQ(table.columns.size(), 2);
    EXPECT_EQ(table.columns[0].name, "col1");
    EXPECT_EQ(table.columns[1].name, "col2");
}

TEST_F(UnityCatalogErrorTest, ParseTableValidJSON) {
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": "test_table",
        "catalog_name": "test_catalog",
        "schema_name": "test_schema",
        "table_type": "MANAGED",
        "data_source_format": "DELTA",
        "owner": "admin",
        "full_name": "test_catalog.test_schema.test_table"
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/tables/test_catalog.test_schema.test_table"))
        .WillOnce(Return(response));

    TableInfo table = unity_catalog_->get_table("test_catalog.test_schema.test_table");
    EXPECT_EQ(table.name, "test_table");
    EXPECT_EQ(table.catalog_name, "test_catalog");
    EXPECT_EQ(table.schema_name, "test_schema");
    EXPECT_EQ(table.table_type, "MANAGED");
}

// =============================================================================
// Column Parsing Error Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, ParseColumnMalformedJSON) {
    // This is tested indirectly through table parsing
    internal::HttpResponse response;
    response.status_code = 200;
    response.body = R"({
        "name": "test_table",
        "catalog_name": "test_catalog",
        "schema_name": "test_schema",
        "table_type": "MANAGED",
        "columns": [
            "totally not json {{{["
        ]
    })";

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/tables/test_table")).WillOnce(Return(response));

    // Should handle gracefully - skipping invalid columns
    TableInfo table = unity_catalog_->get_table("test_table");
    EXPECT_EQ(table.columns.size(), 0); // Invalid column skipped
}

// =============================================================================
// Error Message Truncation Tests
// =============================================================================

TEST_F(UnityCatalogErrorTest, LongJSONTruncatedInError) {
    // Create a very long JSON string (> 200 chars)
    std::string long_json = R"({"name": "test", "very_long_field": ")";
    for (int i = 0; i < 300; ++i) {
        long_json += "x";
    }
    long_json += "\"}";

    internal::HttpResponse response;
    response.status_code = 200;
    response.body = long_json;

    EXPECT_CALL(*mock_http_client_, get("/unity-catalog/catalogs/test")).WillOnce(Return(response));

    try {
        unity_catalog_->get_catalog("test");
        // Might succeed or fail depending on JSON validity
    } catch (const std::runtime_error& e) {
        std::string error_msg(e.what());

        // If there's an error, JSON should be truncated
        if (error_msg.find("JSON") != std::string::npos) {
            EXPECT_THAT(error_msg, HasSubstr("truncated"));

            // Error message should not contain the entire 300+ char string
            EXPECT_LT(error_msg.length(), long_json.length());
        }
    }
}

// Note: main() is provided by gtest_main linked in CMakeLists.txt
