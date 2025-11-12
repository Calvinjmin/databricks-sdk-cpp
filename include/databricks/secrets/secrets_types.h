// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace databricks {

/**
 * @brief Enumeration of secret scope backend types
 */
enum class SecretScopeBackendType {
    DATABRICKS,     ///< Databricks-backed scope (managed in control plane)
    AZURE_KEYVAULT, ///< Azure Key Vault-backed scope (external storage)
    UNKNOWN         ///< Unknown or unrecognized backend type
};

/**
 * @brief Enumeration of secret ACL permission levels
 */
enum class SecretPermission {
    READ,   ///< Read permission - can read secret values
    WRITE,  ///< Write permission - can write/update secrets
    MANAGE, ///< Manage permission - full control including ACL management
    UNKNOWN ///< Unknown permission level
};

/**
 * @brief Represents a Secret Scope
 */
struct SecretScope {
    std::string name;                                                         ///< Name of the secret scope
    SecretScopeBackendType backend_type = SecretScopeBackendType::DATABRICKS; ///< Backend storage type
    std::string resource_id; ///< Azure Key Vault resource ID (for AZURE_KEYVAULT type)
    std::string dns_name;    ///< Azure Key Vault DNS name (for AZURE_KEYVAULT type)

    /**
     * @brief Parse a SecretScope from JSON string
     * @param json_str JSON representation of a secret scope
     * @return Parsed SecretScope object
     * @throws std::runtime_error if parsing fails
     */
    static SecretScope from_json(const std::string& json_str);
};

/**
 * @brief Represents secret metadata (not the secret value)
 */
struct Secret {
    std::string key;                     ///< Secret key name/identifier
    uint64_t last_updated_timestamp = 0; ///< Unix timestamp in milliseconds of last update

    /**
     * @brief Parse a Secret from JSON string
     * @param json_str JSON representation of a secret metadata
     * @return Parsed Secret object
     * @throws std::runtime_error if parsing fails
     */
    static Secret from_json(const std::string& json_str);
};

/**
 * @brief Represents an Access Control List entry for a secret scope
 */
struct SecretACL {
    std::string principal;                                ///< User or group name
    SecretPermission permission = SecretPermission::READ; ///< Permission level (READ, WRITE, MANAGE)

    /**
     * @brief Parse a SecretACL from JSON string
     * @param json_str JSON representation of a secret ACL
     * @return Parsed SecretACL object
     * @throws std::runtime_error if parsing fails
     */
    static SecretACL from_json(const std::string& json_str);
};

} // namespace databricks
