// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
/**
 * @file workspace_example.cpp
 * @brief Example demonstrating the Databricks Workspace API
 *
 * This example shows how to:
 * 1. List workspace objects in a directory
 * 2. Create a new directory
 * 3. Get object status/metadata
 * 4. Import a simple Python notebook
 * 5. Export the notebook
 * 6. Delete objects and clean up
 */

#include "databricks/core/config.h"
#include "databricks/workspace/workspace.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>

// Helper function to encode string to base64
std::string base64_encode(const std::string& input) {
    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "0123456789+/";

    std::string encoded;
    int val = 0;
    int valb = -6;

    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (encoded.size() % 4) {
        encoded.push_back('=');
    }

    return encoded;
}

// Helper function to print ObjectType
std::string object_type_to_string(databricks::ObjectType type) {
    switch (type) {
    case databricks::ObjectType::NOTEBOOK:
        return "NOTEBOOK";
    case databricks::ObjectType::DIRECTORY:
        return "DIRECTORY";
    case databricks::ObjectType::LIBRARY:
        return "LIBRARY";
    case databricks::ObjectType::FILE:
        return "FILE";
    case databricks::ObjectType::REPO:
        return "REPO";
    default:
        return "UNKNOWN";
    }
}

// Helper function to print Language
std::string language_to_string(databricks::Language lang) {
    switch (lang) {
    case databricks::Language::PYTHON:
        return "PYTHON";
    case databricks::Language::SCALA:
        return "SCALA";
    case databricks::Language::SQL:
        return "SQL";
    case databricks::Language::R:
        return "R";
    default:
        return "UNKNOWN";
    }
}

int main() {
    try {
        // Load configuration from environment
        databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

        std::cout << "Connecting to: " << auth.host << std::endl;
        std::cout << "======================================\n" << std::endl;

        // Create Workspace API client
        databricks::Workspace workspace(auth);

        // Use the user's home directory - replace with your Databricks username
        // Example: /Users/yourname@company.com
        std::string base_path = "/Users";

        std::cout << "NOTE: This example uses path: " << base_path << std::endl;
        std::cout << "You may need to change this to your user directory.\n" << std::endl;

        // ===================================================================
        // Example 1: List workspace objects in the /Users directory
        // ===================================================================
        std::cout << "1. Listing workspace objects in '" << base_path << "':" << std::endl;
        std::cout << "---------------------------------------------------" << std::endl;

        auto objects = workspace.list(base_path, std::optional<uint64_t>{});
        std::cout << "Found " << objects.size() << " objects:\n" << std::endl;

        for (size_t i = 0; i < std::min(objects.size(), size_t(5)); ++i) {
            const auto& obj = objects[i];
            std::cout << "  Path:        " << obj.path << std::endl;
            std::cout << "  Type:        " << object_type_to_string(obj.object_type) << std::endl;
            std::cout << "  Object ID:   " << obj.object_id << std::endl;
            if (obj.object_type == databricks::ObjectType::NOTEBOOK) {
                std::cout << "  Language:    " << language_to_string(obj.language) << std::endl;
            }
            std::cout << std::endl;
        }

        if (objects.size() > 5) {
            std::cout << "  ... and " << (objects.size() - 5) << " more objects\n" << std::endl;
        }

        // Find a user directory to work with
        std::string user_path;
        for (const auto& obj : objects) {
            if (obj.object_type == databricks::ObjectType::DIRECTORY) {
                user_path = obj.path;
                break;
            }
        }

        if (user_path.empty()) {
            std::cout << "\nNo user directories found. Exiting example." << std::endl;
            return 0;
        }

        std::cout << "Using directory: " << user_path << std::endl;

        // ===================================================================
        // Example 2: Create a new directory
        // ===================================================================
        std::cout << "\n2. Creating a new directory:" << std::endl;
        std::cout << "----------------------------" << std::endl;

        std::string example_dir = user_path + "/sdk_example";
        std::cout << "Creating directory: " << example_dir << std::endl;

        workspace.mkdirs(example_dir);
        std::cout << "Directory created successfully!\n" << std::endl;

        // ===================================================================
        // Example 3: Get object status/metadata
        // ===================================================================
        std::cout << "\n3. Getting status of the created directory:" << std::endl;
        std::cout << "-------------------------------------------" << std::endl;

        auto dir_info = workspace.get_status(example_dir);
        std::cout << "  Path:        " << dir_info.path << std::endl;
        std::cout << "  Type:        " << object_type_to_string(dir_info.object_type) << std::endl;
        std::cout << "  Object ID:   " << dir_info.object_id << std::endl;
        std::cout << std::endl;

        // ===================================================================
        // Example 4: Import a simple Python notebook
        // ===================================================================
        std::cout << "\n4. Importing a Python notebook:" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        // Create a simple Python notebook content
        std::string notebook_content = "# Databricks notebook source\n"
                                       "# MAGIC %md\n"
                                       "# MAGIC # Example Notebook\n"
                                       "# MAGIC This notebook was created using the Databricks C++ SDK\n"
                                       "\n"
                                       "# COMMAND ----------\n"
                                       "\n"
                                       "print(\"Hello from Databricks C++ SDK!\")\n"
                                       "\n"
                                       "# COMMAND ----------\n"
                                       "\n"
                                       "# Sample data processing\n"
                                       "data = [1, 2, 3, 4, 5]\n"
                                       "squared = [x**2 for x in data]\n"
                                       "print(f\"Original: {data}\")\n"
                                       "print(f\"Squared:  {squared}\")\n";

        // Base64 encode the content
        std::string encoded_content = base64_encode(notebook_content);

        std::string notebook_path = example_dir + "/example_notebook";
        std::cout << "Importing notebook to: " << notebook_path << std::endl;

        workspace.import_file(notebook_path, encoded_content, databricks::ImportFormat::SOURCE,
                              std::optional<databricks::Language>(databricks::Language::PYTHON),
                              true // overwrite if exists
        );

        std::cout << "Notebook imported successfully!\n" << std::endl;

        // ===================================================================
        // Example 5: Export the notebook
        // ===================================================================
        std::cout << "\n5. Exporting the notebook:" << std::endl;
        std::cout << "--------------------------" << std::endl;

        auto export_response = workspace.export_file(notebook_path, databricks::ExportFormat::SOURCE);

        std::cout << "  Notebook exported successfully!" << std::endl;
        std::cout << "  File type:     " << export_response.file_type << std::endl;
        std::cout << "  Content size:  " << export_response.content.size() << " bytes (base64)" << std::endl;
        std::cout << std::endl;

        // ===================================================================
        // Example 6: List objects in our example directory
        // ===================================================================
        std::cout << "\n6. Listing objects in '" << example_dir << "':" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        auto example_objects = workspace.list(example_dir, std::nullopt);
        std::cout << "Found " << example_objects.size() << " objects:\n" << std::endl;

        for (const auto& obj : example_objects) {
            std::cout << "  Path:        " << obj.path << std::endl;
            std::cout << "  Type:        " << object_type_to_string(obj.object_type) << std::endl;
            if (obj.object_type == databricks::ObjectType::NOTEBOOK) {
                std::cout << "  Language:    " << language_to_string(obj.language) << std::endl;
            }
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 7: Cleanup - Delete objects
        // ===================================================================
        std::cout << "\n7. Cleaning up (deleting notebook and directory):" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;

        std::cout << "Deleting notebook: " << notebook_path << std::endl;
        workspace.delete_object(notebook_path, false);
        std::cout << "Notebook deleted successfully!" << std::endl;

        std::cout << "Deleting directory: " << example_dir << std::endl;
        workspace.delete_object(example_dir, true); // recursive delete
        std::cout << "Directory deleted successfully!\n" << std::endl;

        std::cout << "\n======================================" << std::endl;
        std::cout << "Workspace API example completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
