#include "databricks/unity_catalog/unity_catalog.h"
#include "../internal/http_client.h"
#include "../internal/http_client_interface.h"
#include "../internal/logger.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace databricks {
    // ==================== PIMPL IMPLEMENTATION ====================

    class UnityCatalog::Impl {
    public:
        // Constructor for production use (creates real HttpClient)
        explicit Impl(const AuthConfig& auth)
            : http_client_(std::make_shared<internal::HttpClient>(auth)) {}

        // Constructor for testing (accepts injected client)
        explicit Impl(std::shared_ptr<internal::IHttpClient> client)
            : http_client_(std::move(client)) {}

        std::shared_ptr<internal::IHttpClient> http_client_;
    };

    // ==================== CONSTRUCTORS & DESTRUCTOR ====================

    UnityCatalog::UnityCatalog(const AuthConfig& auth)
        : pimpl_(std::make_unique<Impl>(auth)) {}

    UnityCatalog::UnityCatalog(std::shared_ptr<internal::IHttpClient> http_client)
        : pimpl_(std::make_unique<Impl>(std::move(http_client))) {}

    UnityCatalog::~UnityCatalog() = default;

    // ==================== CATALOG OPERATIONS ====================

    std::vector<CatalogInfo> UnityCatalog::list_catalogs() {
        internal::get_logger()->info("Listing Unity Catalog catalogs");

        // TODO: Make API request to /api/2.1/unity-catalog/catalogs
        auto response = pimpl_->http_client_->get("/api/2.1/unity-catalog/catalogs");
        pimpl_->http_client_->check_response(response, "listCatalogs");

        internal::get_logger()->debug("Catalogs list response: " + response.body);
        return parse_catalog_list(response.body);
    }

    CatalogInfo UnityCatalog::get_catalog(const std::string& catalog_name) {
        internal::get_logger()->info("Getting catalog details for catalog=" + catalog_name);

        // TODO: Make API request to /api/2.1/unity-catalog/catalogs/{name}
        auto response = pimpl_->http_client_->get("/api/2.1/unity-catalog/catalogs/" + catalog_name);
        pimpl_->http_client_->check_response(response, "getCatalog");

        internal::get_logger()->debug("Catalog details response: " + response.body);
        return parse_catalog(response.body);
    }

    CatalogInfo UnityCatalog::create_catalog(const CreateCatalogRequest& request) {
        internal::get_logger()->info("Creating catalog: " + request.name);

        // TODO: Build JSON body from CreateCatalogRequest
        json body_json;
        body_json["name"] = request.name;

        if (!request.comment.empty()) {
            body_json["comment"] = request.comment;
        }

        if (!request.properties.empty()) {
            body_json["properties"] = request.properties;
        }

        if (request.storage_root.has_value()) {
            body_json["storage_root"] = request.storage_root.value();
        }

        std::string body = body_json.dump();
        internal::get_logger()->debug("Create catalog request body: " + body);

        // TODO: Make API request to /api/2.1/unity-catalog/catalogs
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/catalogs", body);
        pimpl_->http_client_->check_response(response, "createCatalog");

        internal::get_logger()->info("Successfully created catalog: " + request.name);
        return parse_catalog(response.body);
    }

    CatalogInfo UnityCatalog::update_catalog(const UpdateCatalogRequest& request) {
        internal::get_logger()->info("Updating catalog: " + request.name);

        // TODO: Build JSON body from UpdateCatalogRequest
        json body_json;

        if (request.new_name.has_value()) {
            body_json["new_name"] = request.new_name.value();
        }

        if (request.comment.has_value()) {
            body_json["comment"] = request.comment.value();
        }

        if (request.owner.has_value()) {
            body_json["owner"] = request.owner.value();
        }

        if (!request.properties.empty()) {
            body_json["properties"] = request.properties;
        }

        std::string body = body_json.dump();
        internal::get_logger()->debug("Update catalog request body: " + body);

        // TODO: Make API request to /api/2.1/unity-catalog/catalogs/{name} (PATCH)
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/catalogs/" + request.name, body);
        pimpl_->http_client_->check_response(response, "updateCatalog");

        internal::get_logger()->info("Successfully updated catalog: " + request.name);
        return parse_catalog(response.body);
    }

    bool UnityCatalog::delete_catalog(const std::string& catalog_name, bool force) {
        internal::get_logger()->info("Deleting catalog: " + catalog_name);

        // TODO: Build endpoint with force parameter if needed
        std::string endpoint = "/api/2.1/unity-catalog/catalogs/" + catalog_name;
        if (force) {
            endpoint += "?force=true";
        }

        internal::get_logger()->debug("Delete catalog endpoint: " + endpoint);

        // TODO: Make DELETE API request (using post with empty body for now)
        auto response = pimpl_->http_client_->post(endpoint, "");
        pimpl_->http_client_->check_response(response, "deleteCatalog");

        internal::get_logger()->info("Successfully deleted catalog: " + catalog_name);
        return true;
    }

    // ==================== SCHEMA OPERATIONS ====================

    std::vector<SchemaInfo> UnityCatalog::list_schemas(const std::string& catalog_name) {
        internal::get_logger()->info("Listing schemas in catalog: " + catalog_name);

        // TODO: Make API request to /api/2.1/unity-catalog/schemas?catalog_name={catalog}
        auto response = pimpl_->http_client_->get("/api/2.1/unity-catalog/schemas?catalog_name=" + catalog_name);
        pimpl_->http_client_->check_response(response, "listSchemas");

        internal::get_logger()->debug("Schemas list response: " + response.body);
        return parse_schema_list(response.body);
    }

    SchemaInfo UnityCatalog::get_schema(const std::string& full_name) {
        internal::get_logger()->info("Getting schema details for: " + full_name);

        // TODO: Make API request to /api/2.1/unity-catalog/schemas/{full_name}
        auto response = pimpl_->http_client_->get("/api/2.1/unity-catalog/schemas/" + full_name);
        pimpl_->http_client_->check_response(response, "getSchema");

        internal::get_logger()->debug("Schema details response: " + response.body);
        return parse_schema(response.body);
    }

    SchemaInfo UnityCatalog::create_schema(const CreateSchemaRequest& request) {
        internal::get_logger()->info("Creating schema: " + request.catalog_name + "." + request.name);

        // TODO: Build JSON body from CreateSchemaRequest
        json body_json;
        body_json["name"] = request.name;
        body_json["catalog_name"] = request.catalog_name;

        if (!request.comment.empty()) {
            body_json["comment"] = request.comment;
        }

        if (!request.properties.empty()) {
            body_json["properties"] = request.properties;
        }

        if (request.storage_root.has_value()) {
            body_json["storage_root"] = request.storage_root.value();
        }

        std::string body = body_json.dump();
        internal::get_logger()->debug("Create schema request body: " + body);

        // TODO: Make API request to /api/2.1/unity-catalog/schemas
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/schemas", body);
        pimpl_->http_client_->check_response(response, "createSchema");

        internal::get_logger()->info("Successfully created schema: " + request.catalog_name + "." + request.name);
        return parse_schema(response.body);
    }

    SchemaInfo UnityCatalog::update_schema(const UpdateSchemaRequest& request) {
        internal::get_logger()->info("Updating schema: " + request.full_name);

        // TODO: Build JSON body from UpdateSchemaRequest
        json body_json;

        if (request.new_name.has_value()) {
            body_json["new_name"] = request.new_name.value();
        }

        if (request.comment.has_value()) {
            body_json["comment"] = request.comment.value();
        }

        if (request.owner.has_value()) {
            body_json["owner"] = request.owner.value();
        }

        if (!request.properties.empty()) {
            body_json["properties"] = request.properties;
        }

        std::string body = body_json.dump();
        internal::get_logger()->debug("Update schema request body: " + body);

        // TODO: Make API request to /api/2.1/unity-catalog/schemas/{full_name} (PATCH)
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/schemas/" + request.full_name, body);
        pimpl_->http_client_->check_response(response, "updateSchema");

        internal::get_logger()->info("Successfully updated schema: " + request.full_name);
        return parse_schema(response.body);
    }

    bool UnityCatalog::delete_schema(const std::string& full_name) {
        internal::get_logger()->info("Deleting schema: " + full_name);

        // TODO: Make DELETE API request to /api/2.1/unity-catalog/schemas/{full_name}
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/schemas/" + full_name, "");
        pimpl_->http_client_->check_response(response, "deleteSchema");

        internal::get_logger()->info("Successfully deleted schema: " + full_name);
        return true;
    }

    // ==================== TABLE OPERATIONS ====================

    std::vector<TableInfo> UnityCatalog::list_tables(const std::string& catalog_name,
                                                     const std::string& schema_name) {
        internal::get_logger()->info("Listing tables in " + catalog_name + "." + schema_name);

        // TODO: Make API request to /api/2.1/unity-catalog/tables?catalog_name={catalog}&schema_name={schema}
        std::string endpoint = "/api/2.1/unity-catalog/tables?catalog_name=" + catalog_name +
                              "&schema_name=" + schema_name;
        auto response = pimpl_->http_client_->get(endpoint);
        pimpl_->http_client_->check_response(response, "listTables");

        internal::get_logger()->debug("Tables list response: " + response.body);
        return parse_table_list(response.body);
    }

    TableInfo UnityCatalog::get_table(const std::string& full_name) {
        internal::get_logger()->info("Getting table details for: " + full_name);

        // TODO: Make API request to /api/2.1/unity-catalog/tables/{full_name}
        auto response = pimpl_->http_client_->get("/api/2.1/unity-catalog/tables/" + full_name);
        pimpl_->http_client_->check_response(response, "getTable");

        internal::get_logger()->debug("Table details response: " + response.body);
        return parse_table(response.body);
    }

    bool UnityCatalog::delete_table(const std::string& full_name) {
        internal::get_logger()->info("Deleting table: " + full_name);

        // TODO: Make DELETE API request to /api/2.1/unity-catalog/tables/{full_name}
        auto response = pimpl_->http_client_->post("/api/2.1/unity-catalog/tables/" + full_name, "");
        pimpl_->http_client_->check_response(response, "deleteTable");

        internal::get_logger()->info("Successfully deleted table: " + full_name);
        return true;
    }

    // ==================== PRIVATE PARSING METHODS ====================

    CatalogInfo UnityCatalog::parse_catalog(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            CatalogInfo catalog;

            // TODO: Parse all fields from JSON response
            catalog.name = j.value("name", "");
            catalog.comment = j.value("comment", "");
            catalog.owner = j.value("owner", "");
            catalog.catalog_type = j.value("catalog_type", "");
            catalog.created_at = j.value("created_at", uint64_t(0));
            catalog.updated_at = j.value("updated_at", uint64_t(0));
            catalog.metastore_id = j.value("metastore_id", "");
            catalog.full_name = j.value("full_name", "");

            // Parse properties if present
            if (j.contains("properties") && j["properties"].is_object()) {
                for (auto& [key, value] : j["properties"].items()) {
                    if (value.is_string()) {
                        catalog.properties[key] = value.get<std::string>();
                    }
                }
            }

            // Parse optional fields
            if (j.contains("storage_root") && !j["storage_root"].is_null()) {
                catalog.storage_root = j["storage_root"].get<std::string>();
            }

            if (j.contains("storage_location") && !j["storage_location"].is_null()) {
                catalog.storage_location = j["storage_location"].get<std::string>();
            }

            return catalog;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Catalog JSON: " + std::string(e.what()));
        }
    }

    std::vector<CatalogInfo> UnityCatalog::parse_catalog_list(const std::string& json_str) {
        std::vector<CatalogInfo> catalogs;

        try {
            auto j = json::parse(json_str);

            // TODO: Parse catalogs array from response
            if (!j.contains("catalogs") || !j["catalogs"].is_array()) {
                internal::get_logger()->warn("No catalogs array found in response");
                return catalogs;
            }

            for (const auto& catalog_json : j["catalogs"]) {
                catalogs.push_back(parse_catalog(catalog_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(catalogs.size()) + " catalogs");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse catalogs list: " + std::string(e.what()));
            throw std::runtime_error("Failed to parse catalogs list: " + std::string(e.what()));
        }

        return catalogs;
    }

    SchemaInfo UnityCatalog::parse_schema(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            SchemaInfo schema;

            // TODO: Parse all fields from JSON response
            schema.name = j.value("name", "");
            schema.catalog_name = j.value("catalog_name", "");
            schema.comment = j.value("comment", "");
            schema.owner = j.value("owner", "");
            schema.created_at = j.value("created_at", uint64_t(0));
            schema.updated_at = j.value("updated_at", uint64_t(0));
            schema.metastore_id = j.value("metastore_id", "");
            schema.full_name = j.value("full_name", "");

            // Parse properties if present
            if (j.contains("properties") && j["properties"].is_object()) {
                for (auto& [key, value] : j["properties"].items()) {
                    if (value.is_string()) {
                        schema.properties[key] = value.get<std::string>();
                    }
                }
            }

            // Parse optional fields
            if (j.contains("storage_root") && !j["storage_root"].is_null()) {
                schema.storage_root = j["storage_root"].get<std::string>();
            }

            if (j.contains("storage_location") && !j["storage_location"].is_null()) {
                schema.storage_location = j["storage_location"].get<std::string>();
            }

            return schema;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Schema JSON: " + std::string(e.what()));
        }
    }

    std::vector<SchemaInfo> UnityCatalog::parse_schema_list(const std::string& json_str) {
        std::vector<SchemaInfo> schemas;

        try {
            auto j = json::parse(json_str);

            // TODO: Parse schemas array from response
            if (!j.contains("schemas") || !j["schemas"].is_array()) {
                internal::get_logger()->warn("No schemas array found in response");
                return schemas;
            }

            for (const auto& schema_json : j["schemas"]) {
                schemas.push_back(parse_schema(schema_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(schemas.size()) + " schemas");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse schemas list: " + std::string(e.what()));
            throw std::runtime_error("Failed to parse schemas list: " + std::string(e.what()));
        }

        return schemas;
    }

    ColumnInfo UnityCatalog::parse_column(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            ColumnInfo column;

            // TODO: Parse all fields from JSON response
            column.name = j.value("name", "");
            column.type_text = j.value("type_text", "");
            column.type_name = j.value("type_name", "");
            column.position = j.value("position", 0);
            column.comment = j.value("comment", "");
            column.nullable = j.value("nullable", true);

            // Parse optional partition index
            if (j.contains("partition_index") && !j["partition_index"].is_null()) {
                column.partition_index = j["partition_index"].get<std::string>();
            }

            return column;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Column JSON: " + std::string(e.what()));
        }
    }

    TableInfo UnityCatalog::parse_table(const std::string& json_str) {
        try {
            auto j = json::parse(json_str);
            TableInfo table;

            // TODO: Parse all fields from JSON response
            table.name = j.value("name", "");
            table.catalog_name = j.value("catalog_name", "");
            table.schema_name = j.value("schema_name", "");
            table.table_type = j.value("table_type", "");
            table.data_source_format = j.value("data_source_format", "");
            table.comment = j.value("comment", "");
            table.owner = j.value("owner", "");
            table.created_at = j.value("created_at", uint64_t(0));
            table.updated_at = j.value("updated_at", uint64_t(0));
            table.metastore_id = j.value("metastore_id", "");
            table.full_name = j.value("full_name", "");

            // Parse optional storage location
            if (j.contains("storage_location") && !j["storage_location"].is_null()) {
                table.storage_location = j["storage_location"].get<std::string>();
            }

            // Parse properties if present
            if (j.contains("properties") && j["properties"].is_object()) {
                for (auto& [key, value] : j["properties"].items()) {
                    if (value.is_string()) {
                        table.properties[key] = value.get<std::string>();
                    }
                }
            }

            // Parse columns if present
            if (j.contains("columns") && j["columns"].is_array()) {
                for (const auto& col_json : j["columns"]) {
                    table.columns.push_back(parse_column(col_json.dump()));
                }
            }

            // Parse optional view definition
            if (j.contains("view_definition") && !j["view_definition"].is_null()) {
                table.view_definition = j["view_definition"].get<std::string>();
            }

            // Parse optional table_id
            if (j.contains("table_id") && !j["table_id"].is_null()) {
                table.table_id = j["table_id"].get<uint64_t>();
            }

            return table;
        } catch (const json::exception& e) {
            throw std::runtime_error("Failed to parse Table JSON: " + std::string(e.what()));
        }
    }

    std::vector<TableInfo> UnityCatalog::parse_table_list(const std::string& json_str) {
        std::vector<TableInfo> tables;

        try {
            auto j = json::parse(json_str);

            // TODO: Parse tables array from response
            if (!j.contains("tables") || !j["tables"].is_array()) {
                internal::get_logger()->warn("No tables array found in response");
                return tables;
            }

            for (const auto& table_json : j["tables"]) {
                tables.push_back(parse_table(table_json.dump()));
            }

            internal::get_logger()->info("Parsed " + std::to_string(tables.size()) + " tables");
        } catch (const json::exception& e) {
            internal::get_logger()->error("Failed to parse tables list: " + std::string(e.what()));
            throw std::runtime_error("Failed to parse tables list: " + std::string(e.what()));
        }

        return tables;
    }

} // namespace databricks
