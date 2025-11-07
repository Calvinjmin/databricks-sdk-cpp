// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
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
    // Constructor for production use (creates real HttpClient with Unity Catalog API version)
    explicit Impl(const AuthConfig& auth, const std::string& api_version = "2.1")
        : http_client_(std::make_shared<internal::HttpClient>(auth, api_version)) {}

    // Constructor for testing (accepts injected client)
    explicit Impl(std::shared_ptr<internal::IHttpClient> client)
        : http_client_(std::move(client)) {}

    std::shared_ptr<internal::IHttpClient> http_client_;
};

// ==================== CONSTRUCTORS & DESTRUCTOR ====================

UnityCatalog::UnityCatalog(const AuthConfig& auth, const std::string& api_version)
    : pimpl_(std::make_unique<Impl>(auth, api_version)) {}

UnityCatalog::UnityCatalog(std::shared_ptr<internal::IHttpClient> http_client)
    : pimpl_(std::make_unique<Impl>(std::move(http_client))) {}

UnityCatalog::~UnityCatalog() = default;

// ==================== CATALOG OPERATIONS ====================

std::vector<CatalogInfo> UnityCatalog::list_catalogs() {
    internal::get_logger()->info("Listing Unity Catalog catalogs");

    auto response = pimpl_->http_client_->get("/unity-catalog/catalogs");
    pimpl_->http_client_->check_response(response, "listCatalogs");

    internal::get_logger()->debug("Catalogs list response: " + response.body);
    return parse_catalog_list(response.body);
}

CatalogInfo UnityCatalog::get_catalog(const std::string& catalog_name) {
    internal::get_logger()->info("Getting catalog details for catalog=" + catalog_name);

    auto response = pimpl_->http_client_->get("/unity-catalog/catalogs/" + catalog_name);
    pimpl_->http_client_->check_response(response, "getCatalog");

    internal::get_logger()->debug("Catalog details response: " + response.body);
    return parse_catalog(response.body);
}

CatalogInfo UnityCatalog::create_catalog(const CreateCatalogRequest& request) {
    internal::get_logger()->info("Creating catalog: " + request.name);

    json body_json = request;
    std::string body = body_json.dump();
    internal::get_logger()->debug("Create catalog request body: " + body);

    auto response = pimpl_->http_client_->post("/unity-catalog/catalogs", body);
    pimpl_->http_client_->check_response(response, "createCatalog");

    internal::get_logger()->info("Successfully created catalog: " + request.name);
    return parse_catalog(response.body);
}

CatalogInfo UnityCatalog::update_catalog(const UpdateCatalogRequest& request) {
    internal::get_logger()->info("Updating catalog: " + request.name);

    json body_json = request;
    std::string body = body_json.dump();
    internal::get_logger()->debug("Update catalog request body: " + body);

    auto response = pimpl_->http_client_->post("/unity-catalog/catalogs/" + request.name, body);
    pimpl_->http_client_->check_response(response, "updateCatalog");

    internal::get_logger()->info("Successfully updated catalog: " + request.name);
    return parse_catalog(response.body);
}

bool UnityCatalog::delete_catalog(const std::string& catalog_name, bool force) {
    internal::get_logger()->info("Deleting catalog: " + catalog_name);

    // Force Delete Endpoint
    std::string endpoint = "/api/2.1/unity-catalog/catalogs/" + catalog_name;
    if (force) {
        endpoint += "?force=true";
    }

    internal::get_logger()->debug("Delete catalog endpoint: " + endpoint);

    auto response = pimpl_->http_client_->post(endpoint, "");
    pimpl_->http_client_->check_response(response, "deleteCatalog");

    internal::get_logger()->info("Successfully deleted catalog: " + catalog_name);
    return true;
}

// ==================== SCHEMA OPERATIONS ====================

std::vector<SchemaInfo> UnityCatalog::list_schemas(const std::string& catalog_name) {
    internal::get_logger()->info("Listing schemas in catalog: " + catalog_name);

    auto response = pimpl_->http_client_->get("/unity-catalog/schemas?catalog_name=" + catalog_name);
    pimpl_->http_client_->check_response(response, "listSchemas");

    internal::get_logger()->debug("Schemas list response: " + response.body);
    return parse_schema_list(response.body);
}

SchemaInfo UnityCatalog::get_schema(const std::string& full_name) {
    internal::get_logger()->info("Getting schema details for: " + full_name);

    auto response = pimpl_->http_client_->get("/unity-catalog/schemas/" + full_name);
    pimpl_->http_client_->check_response(response, "getSchema");

    internal::get_logger()->debug("Schema details response: " + response.body);
    return parse_schema(response.body);
}

SchemaInfo UnityCatalog::create_schema(const CreateSchemaRequest& request) {
    internal::get_logger()->info("Creating schema: " + request.catalog_name + "." + request.name);

    json body_json = request;
    std::string body = body_json.dump();
    internal::get_logger()->debug("Create schema request body: " + body);

    auto response = pimpl_->http_client_->post("/unity-catalog/schemas", body);
    pimpl_->http_client_->check_response(response, "createSchema");

    internal::get_logger()->info("Successfully created schema: " + request.catalog_name + "." + request.name);
    return parse_schema(response.body);
}

SchemaInfo UnityCatalog::update_schema(const UpdateSchemaRequest& request) {
    internal::get_logger()->info("Updating schema: " + request.full_name);

    json body_json = request;
    std::string body = body_json.dump();
    internal::get_logger()->debug("Update schema request body: " + body);

    auto response = pimpl_->http_client_->post("/unity-catalog/schemas/" + request.full_name, body);
    pimpl_->http_client_->check_response(response, "updateSchema");

    internal::get_logger()->info("Successfully updated schema: " + request.full_name);
    return parse_schema(response.body);
}

bool UnityCatalog::delete_schema(const std::string& full_name) {
    internal::get_logger()->info("Deleting schema: " + full_name);

    auto response = pimpl_->http_client_->post("/unity-catalog/schemas/" + full_name, "");
    pimpl_->http_client_->check_response(response, "deleteSchema");

    internal::get_logger()->info("Successfully deleted schema: " + full_name);
    return true;
}

// ==================== TABLE OPERATIONS ====================

std::vector<TableInfo> UnityCatalog::list_tables(const std::string& catalog_name, const std::string& schema_name) {
    internal::get_logger()->info("Listing tables in " + catalog_name + "." + schema_name);

    // Create Endpoint with Catalog and Schema name
    std::string endpoint = "/unity-catalog/tables?catalog_name=" + catalog_name + "&schema_name=" + schema_name;
    auto response = pimpl_->http_client_->get(endpoint);
    pimpl_->http_client_->check_response(response, "listTables");

    internal::get_logger()->debug("Tables list response: " + response.body);
    return parse_table_list(response.body);
}

TableInfo UnityCatalog::get_table(const std::string& full_name) {
    internal::get_logger()->info("Getting table details for: " + full_name);

    auto response = pimpl_->http_client_->get("/unity-catalog/tables/" + full_name);
    pimpl_->http_client_->check_response(response, "getTable");

    internal::get_logger()->debug("Table details response: " + response.body);
    return parse_table(response.body);
}

bool UnityCatalog::delete_table(const std::string& full_name) {
    internal::get_logger()->info("Deleting table: " + full_name);

    auto response = pimpl_->http_client_->post("/unity-catalog/tables/" + full_name, "");
    pimpl_->http_client_->check_response(response, "deleteTable");

    internal::get_logger()->info("Successfully deleted table: " + full_name);
    return true;
}

// ==================== PRIVATE PARSING METHODS ====================

CatalogInfo UnityCatalog::parse_catalog(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        CatalogInfo catalog;

        // Validate required fields BEFORE accessing
        std::vector<std::string> missing_fields;
        if (!j.contains("name") || j["name"].is_null()) {
            missing_fields.push_back("name");
        }

        if (!missing_fields.empty()) {
            std::string error = "Missing required fields in Catalog JSON: ";
            for (size_t i = 0; i < missing_fields.size(); ++i) {
                error += missing_fields[i];
                if (i < missing_fields.size() - 1)
                    error += ", ";
            }

            // Include truncated JSON for debugging
            std::string json_preview = json_str.length() > 200 ? json_str.substr(0, 200) + "... (truncated)" : json_str;
            error += "\nJSON received: " + json_preview;

            internal::get_logger()->error(error);
            throw std::runtime_error(error);
        }

        catalog.name = j["name"].get<std::string>();
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
                } else {
                    internal::get_logger()->warn("Skipping non-string property '{}' in catalog '{}'", key,
                                                 catalog.name);
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
    } catch (const json::parse_error& e) {
        // JSON is malformed
        std::string error = "Malformed JSON for Catalog: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::type_error& e) {
        // Field has wrong type
        std::string error = "Type error in Catalog JSON: " + std::string(e.what());
        error += "\nThis usually means a field has unexpected type (e.g., string instead of number)";
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        // Other JSON library errors
        std::string error = "Failed to parse Catalog JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }
}

std::vector<CatalogInfo> UnityCatalog::parse_catalog_list(const std::string& json_str) {
    std::vector<CatalogInfo> catalogs;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("catalogs") || !j["catalogs"].is_array()) {
            internal::get_logger()->warn("No catalogs array found in response");
            return catalogs;
        }

        for (const auto& catalog_json : j["catalogs"]) {
            try {
                catalogs.push_back(parse_catalog(catalog_json.dump()));
            } catch (const std::exception& e) {
                internal::get_logger()->error("Failed to parse individual catalog: {}", e.what());
                // Continue parsing other catalogs instead of failing completely
            }
        }

        internal::get_logger()->info("Parsed {} catalogs", catalogs.size());
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON in catalogs list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse catalogs list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }

    return catalogs;
}

SchemaInfo UnityCatalog::parse_schema(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        SchemaInfo schema;

        // Validate required fields
        std::vector<std::string> missing_fields;
        if (!j.contains("name") || j["name"].is_null()) {
            missing_fields.push_back("name");
        }

        if (!missing_fields.empty()) {
            std::string error = "Missing required fields in Schema JSON: ";
            for (size_t i = 0; i < missing_fields.size(); ++i) {
                error += missing_fields[i];
                if (i < missing_fields.size() - 1)
                    error += ", ";
            }
            error += "\nJSON received: " + json_str.substr(0, std::min(size_t(200), json_str.length()));
            internal::get_logger()->error(error);
            throw std::runtime_error(error);
        }

        schema.name = j["name"].get<std::string>();
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
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON for Schema: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::type_error& e) {
        std::string error = "Type error in Schema JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse Schema JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }
}

std::vector<SchemaInfo> UnityCatalog::parse_schema_list(const std::string& json_str) {
    std::vector<SchemaInfo> schemas;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("schemas") || !j["schemas"].is_array()) {
            internal::get_logger()->warn("No schemas array found in response");
            return schemas;
        }

        for (const auto& schema_json : j["schemas"]) {
            try {
                schemas.push_back(parse_schema(schema_json.dump()));
            } catch (const std::exception& e) {
                internal::get_logger()->error("Failed to parse individual schema: {}", e.what());
                // Continue parsing other schemas
            }
        }

        internal::get_logger()->info("Parsed {} schemas", schemas.size());
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON in schemas list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse schemas list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }

    return schemas;
}

ColumnInfo UnityCatalog::parse_column(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        ColumnInfo column;

        column.name = j.value("name", "");
        column.type_text = j.value("type_text", "");
        column.type_name = j.value("type_name", "");

        // Parse position (can be number or string)
        if (j.contains("position")) {
            if (j["position"].is_number()) {
                column.position = j["position"].get<int>();
            } else if (j["position"].is_string()) {
                try {
                    column.position = std::stoi(j["position"].get<std::string>());
                } catch (const std::exception& e) {
                    internal::get_logger()->warn("Failed to parse position for column '{}': {}", column.name, e.what());
                    column.position = 0;
                }
            }
        }

        column.comment = j.value("comment", "");
        column.nullable = j.value("nullable", true);

        // Parse optional partition index (can be number or string)
        if (j.contains("partition_index") && !j["partition_index"].is_null()) {
            if (j["partition_index"].is_string()) {
                column.partition_index = j["partition_index"].get<std::string>();
            } else if (j["partition_index"].is_number()) {
                column.partition_index = std::to_string(j["partition_index"].get<int>());
            }
        }

        return column;
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON for Column: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse Column JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }
}

TableInfo UnityCatalog::parse_table(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        TableInfo table;

        // Validate required fields
        std::vector<std::string> missing_fields;
        if (!j.contains("name") || j["name"].is_null()) {
            missing_fields.push_back("name");
        }

        if (!missing_fields.empty()) {
            std::string error = "Missing required fields in Table JSON: ";
            for (size_t i = 0; i < missing_fields.size(); ++i) {
                error += missing_fields[i];
                if (i < missing_fields.size() - 1)
                    error += ", ";
            }
            error += "\nJSON received: " + json_str.substr(0, std::min(size_t(200), json_str.length()));
            internal::get_logger()->error(error);
            throw std::runtime_error(error);
        }

        table.name = j["name"].get<std::string>();
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
                try {
                    table.columns.push_back(parse_column(col_json.dump()));
                } catch (const std::exception& e) {
                    internal::get_logger()->warn("Failed to parse column in table '{}': {}", table.name, e.what());
                    // Continue parsing other columns
                }
            }
        }

        // Parse optional view definition
        if (j.contains("view_definition") && !j["view_definition"].is_null()) {
            table.view_definition = j["view_definition"].get<std::string>();
        }

        // Parse optional table_id (can be string or number)
        if (j.contains("table_id") && !j["table_id"].is_null()) {
            if (j["table_id"].is_string()) {
                // Parse string to uint64_t
                try {
                    table.table_id = std::stoull(j["table_id"].get<std::string>());
                } catch (const std::exception& e) {
                    internal::get_logger()->warn("Failed to parse table_id as uint64_t for table '{}': {}", table.name,
                                                 e.what());
                }
            } else if (j["table_id"].is_number()) {
                table.table_id = j["table_id"].get<uint64_t>();
            }
        }

        return table;
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON for Table: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::type_error& e) {
        std::string error = "Type error in Table JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse Table JSON: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }
}

std::vector<TableInfo> UnityCatalog::parse_table_list(const std::string& json_str) {
    std::vector<TableInfo> tables;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("tables") || !j["tables"].is_array()) {
            internal::get_logger()->warn("No tables array found in response");
            return tables;
        }

        for (const auto& table_json : j["tables"]) {
            try {
                tables.push_back(parse_table(table_json.dump()));
            } catch (const std::exception& e) {
                internal::get_logger()->error("Failed to parse individual table: {}", e.what());
                // Continue parsing other tables
            }
        }

        internal::get_logger()->info("Parsed {} tables", tables.size());
    } catch (const json::parse_error& e) {
        std::string error = "Malformed JSON in tables list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    } catch (const json::exception& e) {
        std::string error = "Failed to parse tables list: " + std::string(e.what());
        error += "\nJSON (first 200 chars): " + json_str.substr(0, std::min(size_t(200), json_str.length()));
        internal::get_logger()->error(error);
        throw std::runtime_error(error);
    }

    return tables;
}

} // namespace databricks
