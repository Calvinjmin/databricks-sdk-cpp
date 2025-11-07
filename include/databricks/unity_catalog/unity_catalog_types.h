#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace databricks {

/**
 * @brief Enumeration of catalog types
 */
enum class CatalogTypeEnum {
    MANAGED_CATALOG,  ///< Databricks-managed catalog
    EXTERNAL_CATALOG, ///< External catalog (e.g., AWS Glue, Azure)
    SYSTEM_CATALOG,   ///< System catalog
    UNKNOWN           ///< Unknown catalog type
};

/**
 * @brief Parse a catalog type string into CatalogTypeEnum
 * @param type_str String representation of the catalog type
 * @return CatalogTypeEnum corresponding to the string
 */
CatalogTypeEnum parse_catalog_type(const std::string& type_str);

/**
 * @brief Convert CatalogTypeEnum to string representation
 * @param type CatalogTypeEnum value
 * @return String representation of the catalog type
 */
std::string catalog_type_to_string(CatalogTypeEnum type);

/**
 * @brief Enumeration of table types
 */
enum class TableTypeEnum {
    MANAGED,           ///< Managed table
    EXTERNAL,          ///< External table
    VIEW,              ///< View
    MATERIALIZED_VIEW, ///< Materialized view
    STREAMING_TABLE,   ///< Streaming table
    UNKNOWN            ///< Unknown table type
};

/**
 * @brief Parse a table type string into TableTypeEnum
 * @param type_str String representation of the table type
 * @return TableTypeEnum corresponding to the string
 */
TableTypeEnum parse_table_type(const std::string& type_str);

/**
 * @brief Convert TableTypeEnum to string representation
 * @param type TableTypeEnum value
 * @return String representation of the table type
 */
std::string table_type_to_string(TableTypeEnum type);

/**
 * @brief Represents a Unity Catalog catalog
 *
 * Catalogs are the top-level container for organizing data objects in Unity Catalog.
 */
struct CatalogInfo {
    std::string name;                              ///< Name of the catalog
    std::string comment;                           ///< User-provided description
    std::string owner;                             ///< Owner of the catalog
    std::string catalog_type;                      ///< Type of catalog (MANAGED_CATALOG, etc.)
    uint64_t created_at = 0;                       ///< Unix timestamp in milliseconds when created
    uint64_t updated_at = 0;                       ///< Unix timestamp in milliseconds when last updated
    std::string metastore_id;                      ///< ID of the metastore containing this catalog
    std::string full_name;                         ///< Full name of the catalog
    std::map<std::string, std::string> properties; ///< Catalog properties/metadata
    std::optional<std::string> storage_root;       ///< Storage root location (for external catalogs)
    std::optional<std::string> storage_location;   ///< Storage location (for managed catalogs)
};

/**
 * @brief Represents a Unity Catalog schema
 *
 * Schemas are containers for tables, views, and functions within a catalog.
 */
struct SchemaInfo {
    std::string name;                              ///< Name of the schema
    std::string catalog_name;                      ///< Parent catalog name
    std::string comment;                           ///< User-provided description
    std::string owner;                             ///< Owner of the schema
    uint64_t created_at = 0;                       ///< Unix timestamp in milliseconds when created
    uint64_t updated_at = 0;                       ///< Unix timestamp in milliseconds when last updated
    std::string metastore_id;                      ///< ID of the metastore containing this schema
    std::string full_name;                         ///< Full name (catalog.schema)
    std::map<std::string, std::string> properties; ///< Schema properties/metadata
    std::optional<std::string> storage_root;       ///< Storage root location
    std::optional<std::string> storage_location;   ///< Storage location
};

/**
 * @brief Represents column information
 */
struct ColumnInfo {
    std::string name;                           ///< Column name
    std::string type_text;                      ///< Data type as text
    std::string type_name;                      ///< Type name (e.g., INT, STRING)
    int position = 0;                           ///< Ordinal position in table
    std::string comment;                        ///< Column description
    bool nullable = true;                       ///< Whether column can be null
    std::optional<std::string> partition_index; ///< Partition index if partitioned
};

/**
 * @brief Represents a Unity Catalog table
 *
 * Tables are the primary data storage objects in Unity Catalog.
 */
struct TableInfo {
    std::string name;                              ///< Table name
    std::string catalog_name;                      ///< Parent catalog name
    std::string schema_name;                       ///< Parent schema name
    std::string table_type;                        ///< Type of table (MANAGED, EXTERNAL, VIEW, etc.)
    std::string data_source_format;                ///< Format (DELTA, PARQUET, CSV, etc.)
    std::string comment;                           ///< User-provided description
    std::string owner;                             ///< Owner of the table
    uint64_t created_at = 0;                       ///< Unix timestamp in milliseconds when created
    uint64_t updated_at = 0;                       ///< Unix timestamp in milliseconds when last updated
    std::string metastore_id;                      ///< ID of the metastore containing this table
    std::string full_name;                         ///< Full name (catalog.schema.table)
    std::optional<std::string> storage_location;   ///< Storage location
    std::map<std::string, std::string> properties; ///< Table properties/metadata
    std::vector<ColumnInfo> columns;               ///< Column definitions
    std::optional<std::string> view_definition;    ///< SQL definition for views
    std::optional<uint64_t> table_id;              ///< Unique table identifier
};

/**
 * @brief Configuration for creating a catalog
 */
struct CreateCatalogRequest {
    std::string name;                              ///< Name of the catalog (required)
    std::string comment;                           ///< User-provided description
    std::map<std::string, std::string> properties; ///< Catalog properties/metadata
    std::optional<std::string> storage_root;       ///< Storage root location
};

/**
 * @brief Configuration for updating a catalog
 */
struct UpdateCatalogRequest {
    std::string name;                              ///< Name of the catalog (required)
    std::optional<std::string> new_name;           ///< New name for the catalog
    std::optional<std::string> comment;            ///< Updated description
    std::optional<std::string> owner;              ///< New owner
    std::map<std::string, std::string> properties; ///< Updated properties
};

/**
 * @brief Configuration for creating a schema
 */
struct CreateSchemaRequest {
    std::string name;                              ///< Name of the schema (required)
    std::string catalog_name;                      ///< Parent catalog name (required)
    std::string comment;                           ///< User-provided description
    std::map<std::string, std::string> properties; ///< Schema properties/metadata
    std::optional<std::string> storage_root;       ///< Storage root location
};

/**
 * @brief Configuration for updating a schema
 */
struct UpdateSchemaRequest {
    std::string full_name;                         ///< Full name (catalog.schema) (required)
    std::optional<std::string> new_name;           ///< New name for the schema
    std::optional<std::string> comment;            ///< Updated description
    std::optional<std::string> owner;              ///< New owner
    std::map<std::string, std::string> properties; ///< Updated properties
};

// ==================== JSON SERIALIZATION ====================

/**
 * @brief Convert CreateCatalogRequest to JSON
 */
void to_json(nlohmann::json& j, const CreateCatalogRequest& req);

/**
 * @brief Convert UpdateCatalogRequest to JSON
 */
void to_json(nlohmann::json& j, const UpdateCatalogRequest& req);

/**
 * @brief Convert CreateSchemaRequest to JSON
 */
void to_json(nlohmann::json& j, const CreateSchemaRequest& req);

/**
 * @brief Convert UpdateSchemaRequest to JSON
 */
void to_json(nlohmann::json& j, const UpdateSchemaRequest& req);

} // namespace databricks
