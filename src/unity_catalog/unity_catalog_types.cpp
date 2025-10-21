#include "databricks/unity_catalog/unity_catalog_types.h"

#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

namespace databricks {

    // ==================== CATALOG TYPE ENUM HELPERS ====================

    CatalogTypeEnum parse_catalog_type(const std::string& type_str) {
        // TODO: Implement catalog type parsing
        if (type_str == "MANAGED_CATALOG") {
            return CatalogTypeEnum::MANAGED_CATALOG;
        } else if (type_str == "EXTERNAL_CATALOG") {
            return CatalogTypeEnum::EXTERNAL_CATALOG;
        } else if (type_str == "SYSTEM_CATALOG") {
            return CatalogTypeEnum::SYSTEM_CATALOG;
        }
        return CatalogTypeEnum::UNKNOWN;
    }

    std::string catalog_type_to_string(CatalogTypeEnum type) {
        // TODO: Implement catalog type to string conversion
        switch (type) {
            case CatalogTypeEnum::MANAGED_CATALOG:
                return "MANAGED_CATALOG";
            case CatalogTypeEnum::EXTERNAL_CATALOG:
                return "EXTERNAL_CATALOG";
            case CatalogTypeEnum::SYSTEM_CATALOG:
                return "SYSTEM_CATALOG";
            case CatalogTypeEnum::UNKNOWN:
            default:
                return "UNKNOWN";
        }
    }

    // ==================== TABLE TYPE ENUM HELPERS ====================

    TableTypeEnum parse_table_type(const std::string& type_str) {
        // TODO: Implement table type parsing
        if (type_str == "MANAGED") {
            return TableTypeEnum::MANAGED;
        } else if (type_str == "EXTERNAL") {
            return TableTypeEnum::EXTERNAL;
        } else if (type_str == "VIEW") {
            return TableTypeEnum::VIEW;
        } else if (type_str == "MATERIALIZED_VIEW") {
            return TableTypeEnum::MATERIALIZED_VIEW;
        } else if (type_str == "STREAMING_TABLE") {
            return TableTypeEnum::STREAMING_TABLE;
        }
        return TableTypeEnum::UNKNOWN;
    }

    std::string table_type_to_string(TableTypeEnum type) {
        // TODO: Implement table type to string conversion
        switch (type) {
            case TableTypeEnum::MANAGED:
                return "MANAGED";
            case TableTypeEnum::EXTERNAL:
                return "EXTERNAL";
            case TableTypeEnum::VIEW:
                return "VIEW";
            case TableTypeEnum::MATERIALIZED_VIEW:
                return "MATERIALIZED_VIEW";
            case TableTypeEnum::STREAMING_TABLE:
                return "STREAMING_TABLE";
            case TableTypeEnum::UNKNOWN:
            default:
                return "UNKNOWN";
        }
    }

    // ==================== JSON SERIALIZATION ====================

    void to_json(json& j, const CreateCatalogRequest& req) {
        j = json{{"name", req.name}};

        if (!req.comment.empty()) {
            j["comment"] = req.comment;
        }

        if (!req.properties.empty()) {
            j["properties"] = req.properties;
        }

        if (req.storage_root.has_value()) {
            j["storage_root"] = req.storage_root.value();
        }
    }

    void to_json(json& j, const UpdateCatalogRequest& req) {
        j = json{{"name", req.name}};

        if (req.new_name.has_value()) {
            j["new_name"] = req.new_name.value();
        }

        if (req.comment.has_value()) {
            j["comment"] = req.comment.value();
        }

        if (req.owner.has_value()) {
            j["owner"] = req.owner.value();
        }

        if (!req.properties.empty()) {
            j["properties"] = req.properties;
        }
    }

    void to_json(json& j, const CreateSchemaRequest& req) {
        j = json{
            {"name", req.name},
            {"catalog_name", req.catalog_name}
        };

        if (!req.comment.empty()) {
            j["comment"] = req.comment;
        }

        if (!req.properties.empty()) {
            j["properties"] = req.properties;
        }

        if (req.storage_root.has_value()) {
            j["storage_root"] = req.storage_root.value();
        }
    }

    void to_json(json& j, const UpdateSchemaRequest& req) {
        j = json{{"full_name", req.full_name}};

        if (req.new_name.has_value()) {
            j["new_name"] = req.new_name.value();
        }

        if (req.comment.has_value()) {
            j["comment"] = req.comment.value();
        }

        if (req.owner.has_value()) {
            j["owner"] = req.owner.value();
        }

        if (!req.properties.empty()) {
            j["properties"] = req.properties;
        }
    }

} // namespace databricks
