// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include "databricks/core/config.h"
#include "databricks/secrets/secrets_types.h"

#include <vector>

namespace databricks {
namespace internal {
class IHttpClient;
}

/**
 * @brief Client for interacting with the Databricks Secrets API
 *
 * The Secrets API allows you to securely store and manage credentials, tokens, and other
 * sensitive information in your Databricks workspace. Rather than entering credentials
 * directly into notebooks, you can store them as secrets and reference them securely.
 * This implementation uses Secrets API 2.0.
 *
 * Example usage:
 * @code
 * databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
 * databricks::Secrets secrets(auth);
 *
 * // List all secret scopes
 * auto scopes = secrets.list_scopes();
 *
 * // Create a new secret scope
 * secrets.create_scope("my_scope", databricks::SecretScopeBackendType::DATABRICKS);
 *
 * // Store a secret
 * secrets.put_secret("my_scope", "api_key", "my_secret_value");
 *
 * // List secrets in a scope
 * auto secret_list = secrets.list_secrets("my_scope");
 *
 * // Delete a secret
 * secrets.delete_secret("my_scope", "api_key");
 * @endcode
 */
class Secrets {
public:
    /**
     * @brief Construct a Secrets API client
     * @param auth Authentication configuration with host and token
     */
    explicit Secrets(const AuthConfig& auth);

    /**
     * @brief Construct a Secrets API client with dependency injection (for testing)
     * @param http_client Injected HTTP client (use MockHttpClient for unit tests)
     * @note This constructor is primarily for testing with mock HTTP clients
     */
    explicit Secrets(std::shared_ptr<internal::IHttpClient> http_client);

    /**
     * @brief Destructor
     */
    ~Secrets();

    Secrets(const Secrets&) = delete;
    Secrets& operator=(const Secrets&) = delete;

    // Scope operations
    /**
     * @brief List all secret scopes in the workspace
     *
     * @return Vector of SecretScope objects
     * @throws std::runtime_error if the API request fails
     */
    std::vector<SecretScope> list_scopes();

    /**
     * @brief Create a new secret scope
     *
     * @param scope The name of the secret scope to create
     * @param backend_type The type of backend (DATABRICKS or AZURE_KEYVAULT)
     * @throws std::runtime_error if the scope already exists or the API request fails
     *
     * @note Databricks-backed scopes are stored in the control plane. Azure Key Vault-backed
     *       scopes are stored in your Azure Key Vault instance.
     */
    void create_scope(const std::string& scope, SecretScopeBackendType backend_type);

    /**
     * @brief Delete a secret scope
     *
     * @param scope The name of the secret scope to delete
     * @throws std::runtime_error if the scope is not found or the API request fails
     *
     * @note Deleting a scope also deletes all secrets stored in that scope.
     *       This operation cannot be undone.
     */
    void delete_scope(const std::string& scope);

    // Secret operations
    /**
     * @brief Store a secret as a string value
     *
     * @param scope The name of the secret scope
     * @param key The name of the secret to store
     * @param value The secret value as a string
     * @throws std::runtime_error if the API request fails
     *
     * @note The value parameter is passed by const reference to avoid unnecessary copies.
     *       For enhanced security, users should securely clear the value from memory
     *       after calling this function.
     */
    void put_secret(const std::string& scope, const std::string& key, const std::string& value);

    /**
     * @brief Store a secret as binary data
     *
     * @param scope The name of the secret scope
     * @param key The name of the secret to store
     * @param value The secret value as a byte vector
     * @throws std::runtime_error if the API request fails
     *
     * @note This overload accepts binary/secure values for storing non-text secrets.
     *       Users should securely zero out the value vector after calling this function.
     */
    void put_secret(const std::string& scope, const std::string& key, const std::vector<uint8_t>& value);

    /**
     * @brief Delete a secret from a scope
     *
     * @param scope The name of the secret scope
     * @param key The name of the secret to delete
     * @throws std::runtime_error if the secret is not found or the API request fails
     */
    void delete_secret(const std::string& scope, const std::string& key);

    /**
     * @brief List all secrets in a scope
     *
     * @param scope The name of the secret scope
     * @return Vector of Secret objects containing metadata (not the secret values)
     * @throws std::runtime_error if the API request fails
     *
     * @note This method returns secret metadata only. Secret values cannot be retrieved
     *       via the API for security reasons.
     */
    std::vector<Secret> list_secrets(const std::string& scope);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace databricks