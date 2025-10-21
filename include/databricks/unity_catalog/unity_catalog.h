#pragma once

#include "databricks/core/config.h"
#include "databricks/unity_catalog/unity_catalog_types.h"

#include <string>
#include <vector>
#include <memory>

namespace databricks {
    // Forward declaration for dependency injection
    namespace internal {
        class IHttpClient;
    }

    /**
     * @brief Client for interacting with the Databricks Unity Catalog API
     *
     * Unity Catalog provides a unified governance solution for data and AI assets.
     * This implementation uses Unity Catalog REST API 2.1.
     *
     * Example usage:
     * @code
     * databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
     * databricks::UnityCatalog uc(auth);
     *
     * // List all catalogs
     * auto catalogs = uc.list_catalogs();
     *
     * // Get specific catalog details
     * auto catalog = uc.get_catalog("main");
     *
     * // Create a new catalog
     * databricks::CreateCatalogRequest req;
     * req.name = "my_catalog";
     * req.comment = "My data catalog";
     * uc.create_catalog(req);
     *
     * // List schemas in a catalog
     * auto schemas = uc.list_schemas("main");
     *
     * // List tables in a schema
     * auto tables = uc.list_tables("main", "default");
     * @endcode
     */
    class UnityCatalog {
    public:
        /**
         * @brief Construct a Unity Catalog API client
         * @param auth Authentication configuration with host and token
         */
        explicit UnityCatalog(const AuthConfig& auth);

        /**
         * @brief Construct a Unity Catalog API client with dependency injection (for testing)
         * @param http_client Injected HTTP client (use MockHttpClient for unit tests)
         * @note This constructor is primarily for testing with mock HTTP clients
         */
        explicit UnityCatalog(std::shared_ptr<internal::IHttpClient> http_client);

        /**
         * @brief Destructor
         */
        ~UnityCatalog();

        // Disable copy
        UnityCatalog(const UnityCatalog&) = delete;
        UnityCatalog& operator=(const UnityCatalog&) = delete;

        // ==================== CATALOG OPERATIONS ====================

        /**
         * @brief List all catalogs in the metastore
         *
         * @return Vector of CatalogInfo objects
         * @throws std::runtime_error if the API request fails
         */
        std::vector<CatalogInfo> list_catalogs();

        /**
         * @brief Get detailed information about a specific catalog
         *
         * @param catalog_name The name of the catalog
         * @return CatalogInfo object with full details
         * @throws std::runtime_error if the catalog is not found or the API request fails
         */
        CatalogInfo get_catalog(const std::string& catalog_name);

        /**
         * @brief Create a new catalog
         *
         * @param request Configuration for the new catalog
         * @return CatalogInfo object representing the created catalog
         * @throws std::runtime_error if the API request fails
         */
        CatalogInfo create_catalog(const CreateCatalogRequest& request);

        /**
         * @brief Update an existing catalog
         *
         * @param request Configuration for updating the catalog
         * @return CatalogInfo object representing the updated catalog
         * @throws std::runtime_error if the API request fails
         */
        CatalogInfo update_catalog(const UpdateCatalogRequest& request);

        /**
         * @brief Delete a catalog
         *
         * @param catalog_name The name of the catalog to delete
         * @param force If true, deletes the catalog even if it's not empty
         * @return true if the operation was successful
         * @throws std::runtime_error if the API request fails
         *
         * @note By default, you cannot delete a catalog that contains schemas.
         *       Set force=true to delete a catalog and all its contents.
         */
        bool delete_catalog(const std::string& catalog_name, bool force = false);

        // ==================== SCHEMA OPERATIONS ====================

        /**
         * @brief List all schemas in a catalog
         *
         * @param catalog_name The name of the catalog
         * @return Vector of SchemaInfo objects
         * @throws std::runtime_error if the API request fails
         */
        std::vector<SchemaInfo> list_schemas(const std::string& catalog_name);

        /**
         * @brief Get detailed information about a specific schema
         *
         * @param full_name The full name of the schema (catalog.schema)
         * @return SchemaInfo object with full details
         * @throws std::runtime_error if the schema is not found or the API request fails
         */
        SchemaInfo get_schema(const std::string& full_name);

        /**
         * @brief Create a new schema
         *
         * @param request Configuration for the new schema
         * @return SchemaInfo object representing the created schema
         * @throws std::runtime_error if the API request fails
         */
        SchemaInfo create_schema(const CreateSchemaRequest& request);

        /**
         * @brief Update an existing schema
         *
         * @param request Configuration for updating the schema
         * @return SchemaInfo object representing the updated schema
         * @throws std::runtime_error if the API request fails
         */
        SchemaInfo update_schema(const UpdateSchemaRequest& request);

        /**
         * @brief Delete a schema
         *
         * @param full_name The full name of the schema to delete (catalog.schema)
         * @return true if the operation was successful
         * @throws std::runtime_error if the API request fails
         *
         * @note The schema must be empty (no tables) before deletion
         */
        bool delete_schema(const std::string& full_name);

        // ==================== TABLE OPERATIONS ====================

        /**
         * @brief List all tables in a schema
         *
         * @param catalog_name The name of the catalog
         * @param schema_name The name of the schema
         * @return Vector of TableInfo objects
         * @throws std::runtime_error if the API request fails
         */
        std::vector<TableInfo> list_tables(const std::string& catalog_name,
                                          const std::string& schema_name);

        /**
         * @brief Get detailed information about a specific table
         *
         * @param full_name The full name of the table (catalog.schema.table)
         * @return TableInfo object with full details
         * @throws std::runtime_error if the table is not found or the API request fails
         */
        TableInfo get_table(const std::string& full_name);

        /**
         * @brief Delete a table
         *
         * @param full_name The full name of the table to delete (catalog.schema.table)
         * @return true if the operation was successful
         * @throws std::runtime_error if the API request fails
         *
         * @note For managed tables, this also deletes the underlying data.
         *       For external tables, only the metadata is deleted.
         */
        bool delete_table(const std::string& full_name);

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;

        // Parsing methods
        static CatalogInfo parse_catalog(const std::string& json_str);
        static std::vector<CatalogInfo> parse_catalog_list(const std::string& json_str);
        static SchemaInfo parse_schema(const std::string& json_str);
        static std::vector<SchemaInfo> parse_schema_list(const std::string& json_str);
        static TableInfo parse_table(const std::string& json_str);
        static std::vector<TableInfo> parse_table_list(const std::string& json_str);
        static ColumnInfo parse_column(const std::string& json_str);
    };

} // namespace databricks
