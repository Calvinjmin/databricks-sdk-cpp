// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
/**
 * @file secrets_example.cpp
 * @brief Example demonstrating the Databricks Secrets API
 *
 * This example shows how to:
 * 1. List all secret scopes in your workspace
 * 2. Create a new secret scope
 * 3. Store a secret in the scope
 * 4. List secrets in the scope
 * 5. Delete the secret and scope
 */

#include "databricks/core/config.h"
#include "databricks/secrets/secrets.h"

#include <exception>
#include <iostream>

int main() {
    try {
        // Load configuration from environment
        databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

        std::cout << "Connecting to: " << auth.host << std::endl;
        std::cout << "======================================\n" << std::endl;

        // Create Secrets API client
        databricks::Secrets secrets(auth);

        // ===================================================================
        // Example 1: List all secret scopes
        // ===================================================================
        std::cout << "1. Listing all secret scopes:" << std::endl;
        std::cout << "-----------------------------" << std::endl;

        auto scopes = secrets.list_scopes();
        std::cout << "Found " << scopes.size() << " secret scopes:\n" << std::endl;

        for (const auto& scope : scopes) {
            std::cout << "  Scope Name:   " << scope.name << std::endl;
            std::cout << "  Backend Type: ";
            if (scope.backend_type == databricks::SecretScopeBackendType::DATABRICKS) {
                std::cout << "DATABRICKS" << std::endl;
            } else if (scope.backend_type == databricks::SecretScopeBackendType::AZURE_KEYVAULT) {
                std::cout << "AZURE_KEYVAULT" << std::endl;
                if (!scope.resource_id.empty()) {
                    std::cout << "  Resource ID:  " << scope.resource_id << std::endl;
                }
                if (!scope.dns_name.empty()) {
                    std::cout << "  DNS Name:     " << scope.dns_name << std::endl;
                }
            } else {
                std::cout << "UNKNOWN" << std::endl;
            }
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 2: Create a new secret scope
        // ===================================================================
        std::cout << "\n2. Creating a new secret scope:" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        const std::string example_scope = "example_scope";
        std::cout << "Creating scope: " << example_scope << std::endl;

        // Create scope with "users" as initial_manage_principal
        // This grants MANAGE permission to all workspace users
        secrets.create_scope(example_scope, "users", databricks::SecretScopeBackendType::DATABRICKS);

        std::cout << "Scope created successfully!\n" << std::endl;

        // ===================================================================
        // Example 3: Store a secret
        // ===================================================================
        std::cout << "\n3. Storing a secret:" << std::endl;
        std::cout << "--------------------" << std::endl;

        const std::string secret_key = "api_key";
        const std::string secret_value = "my_secret_value_123";

        std::cout << "Storing secret with key: " << secret_key << std::endl;
        secrets.put_secret(example_scope, secret_key, secret_value);
        std::cout << "Secret stored successfully!\n" << std::endl;

        // ===================================================================
        // Example 4: List secrets in the scope
        // ===================================================================
        std::cout << "\n4. Listing secrets in scope '" << example_scope << "':" << std::endl;
        std::cout << "-----------------------------------------------" << std::endl;

        auto secret_list = secrets.list_secrets(example_scope);
        std::cout << "Found " << secret_list.size() << " secrets:\n" << std::endl;

        for (const auto& secret : secret_list) {
            std::cout << "  Key:              " << secret.key << std::endl;
            std::cout << "  Last Updated:     " << secret.last_updated_timestamp << std::endl;
            std::cout << std::endl;
        }

        // ===================================================================
        // Example 5: Cleanup - Delete secret and scope
        // ===================================================================
        std::cout << "\n5. Cleaning up (deleting secret and scope):" << std::endl;
        std::cout << "-------------------------------------------" << std::endl;

        std::cout << "Deleting secret: " << secret_key << std::endl;
        secrets.delete_secret(example_scope, secret_key);
        std::cout << "Secret deleted successfully!" << std::endl;

        std::cout << "Deleting scope: " << example_scope << std::endl;
        secrets.delete_scope(example_scope);
        std::cout << "Scope deleted successfully!\n" << std::endl;

        std::cout << "\n======================================" << std::endl;
        std::cout << "Secrets API example completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
