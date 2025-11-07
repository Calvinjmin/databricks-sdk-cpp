// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
/**
 * @file unity_catalog_example.cpp
 * @brief Example demonstrating the Databricks Unity Catalog API
 *
 * This example shows how to:
 * 1. List all catalogs in your metastore
 * 2. Get details for a specific catalog
 * 3. List schemas in a catalog
 * 4. List tables in a schema
 */

#include "databricks/core/config.h"
#include "databricks/unity_catalog/unity_catalog.h"

#include <exception>
#include <iostream>

int main() {
    try {
        // Load configuration from environment
        databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

        std::cout << "Connecting to: " << auth.host << std::endl;
        std::cout << "======================================\n" << std::endl;

        // Create Unity Catalog API client (uses API version 2.1 by default)
        databricks::UnityCatalog uc(auth);

        // ===================================================================
        // Example 1: List all catalogs
        // ===================================================================
        std::cout << "1. Listing all catalogs:" << std::endl;
        std::cout << "------------------------" << std::endl;

        auto catalogs = uc.list_catalogs();
        std::cout << "Found " << catalogs.size() << " catalogs:\n" << std::endl;

        for (int i = 0; i < std::min(static_cast<int>(catalogs.size()), 10); i++) {
            const auto& catalog = catalogs[i];
            std::cout << "  Catalog:     " << catalog.name << std::endl;
            std::cout << "  Owner:       " << catalog.owner << std::endl;
            std::cout << "  Type:        " << catalog.catalog_type << std::endl;
            std::cout << "  Metastore:   " << catalog.metastore_id << std::endl;
            if (!catalog.comment.empty()) {
                std::cout << "  Comment:     " << catalog.comment << std::endl;
            }
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 2: Get details for a specific catalog
        // ===================================================================
        if (!catalogs.empty()) {
            std::string catalog_name = catalogs[0].name;

            std::cout << "\n2. Getting details for catalog '" << catalog_name << "':" << std::endl;
            std::cout << "-----------------------------------------------------" << std::endl;

            auto catalog_details = uc.get_catalog(catalog_name);
            std::cout << "  Name:        " << catalog_details.name << std::endl;
            std::cout << "  Full Name:   " << catalog_details.full_name << std::endl;
            std::cout << "  Owner:       " << catalog_details.owner << std::endl;
            std::cout << "  Type:        " << catalog_details.catalog_type << std::endl;
            std::cout << "  Created At:  " << catalog_details.created_at << std::endl;
            std::cout << "  Updated At:  " << catalog_details.updated_at << std::endl;

            if (!catalog_details.properties.empty()) {
                std::cout << "  Properties:" << std::endl;
                for (const auto& [key, value] : catalog_details.properties) {
                    std::cout << "    " << key << ": " << value << std::endl;
                }
            }
            std::cout << std::endl;

            // ===============================================================
            // Example 3: List schemas in the catalog
            // ===============================================================
            std::cout << "\n3. Listing schemas in catalog '" << catalog_name << "':" << std::endl;
            std::cout << "-----------------------------------------------------------" << std::endl;

            auto schemas = uc.list_schemas(catalog_name);
            std::cout << "Found " << schemas.size() << " schemas:\n" << std::endl;

            for (const auto& schema : schemas) {
                std::cout << "  Schema:      " << schema.name << std::endl;
                std::cout << "  Full Name:   " << schema.full_name << std::endl;
                std::cout << "  Owner:       " << schema.owner << std::endl;
                if (!schema.comment.empty()) {
                    std::cout << "  Comment:     " << schema.comment << std::endl;
                }
                std::cout << std::endl;
            }

            // ===============================================================
            // Example 4: List tables in the first schema
            // ===============================================================
            if (!schemas.empty()) {
                std::string schema_name = schemas[0].name;

                std::cout << "\n4. Listing tables in '" << catalog_name << "." << schema_name << "':" << std::endl;
                std::cout << "----------------------------------------------------------------" << std::endl;

                auto tables = uc.list_tables(catalog_name, schema_name);
                std::cout << "Found " << tables.size() << " tables:\n" << std::endl;

                for (const auto& table : tables) {
                    std::cout << "  Table:       " << table.name << std::endl;
                    std::cout << "  Full Name:   " << table.full_name << std::endl;
                    std::cout << "  Type:        " << table.table_type << std::endl;
                    std::cout << "  Format:      " << table.data_source_format << std::endl;
                    std::cout << "  Owner:       " << table.owner << std::endl;
                    std::cout << "  Columns:     " << table.columns.size() << std::endl;
                    if (!table.comment.empty()) {
                        std::cout << "  Comment:     " << table.comment << std::endl;
                    }
                    std::cout << std::endl;
                }
            }
        }

        std::cout << "\n======================================" << std::endl;
        std::cout << "Unity Catalog API example completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
