// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "databricks/secrets/secrets.h"

#include "../internal/http_client.h"
#include "../internal/http_client_interface.h"
#include "../internal/logger.h"

#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

namespace databricks {

// ==================== PIMPL IMPLEMENTATION ====================

class Secrets::Impl {
public:
    // Constructor for production use (creates real HttpClient)
    explicit Impl(const AuthConfig& auth, const std::string& api_version = "2.0")
        : http_client_(std::make_shared<internal::HttpClient>(auth, api_version)) {}

    // Constructor for testing (accepts injected client)
    explicit Impl(std::shared_ptr<internal::IHttpClient> client)
        : http_client_(std::move(client)) {}

    std::shared_ptr<internal::IHttpClient> http_client_;
};

// ==================== CONSTRUCTORS & DESTRUCTOR ====================

Secrets::Secrets(const AuthConfig& auth, const std::string& api_version)
    : pimpl_(std::make_unique<Impl>(auth, api_version)) {}

Secrets::Secrets(std::shared_ptr<internal::IHttpClient> http_client)
    : pimpl_(std::make_unique<Impl>(std::move(http_client))) {}

Secrets::~Secrets() = default;

// ==================== PUBLIC API METHODS ====================

std::vector<SecretScope> Secrets::list_scopes() {
    internal::get_logger()->info("Listing secret scopes");

    auto response = pimpl_->http_client_->get("/secrets/scopes/list");
    pimpl_->http_client_->check_response(response, "listScopes");

    internal::get_logger()->debug("Successfully retrieved secret scopes");
    return parse_scopes_list(response.body);
}

void Secrets::create_scope(SecretPermission secret_permissions,
    const std::string& scope,
    SecretScopeBackendType backend_type,
    const std::optional<std::string>& azure_resource_id,
    const std::optional<std::string>& azure_tenant_id,
    const std::optional<std::string>& dns_name) {

    // Validate Azure parameters if using Azure Key Vault backend
    if (backend_type == SecretScopeBackendType::AZURE_KEYVAULT) {
        if (!azure_resource_id.has_value() || azure_resource_id->empty() ||
            !azure_tenant_id.has_value() || azure_tenant_id->empty() ||
            !dns_name.has_value() || dns_name->empty()) {
            throw std::invalid_argument("Azure resource_id, tenant_id, and dns_name are required for AZURE_KEYVAULT backend");
        }
    }
    
    internal::get_logger()->info("Creating secret scope: " + scope);
    
    // Build JSON Body
    json body_json;
    body_json["scope"] = scope;
    body_json["initial_manage_principal"] = secret_permissions_to_string(secret_permissions);
    body_json["backend_type"] = backend_type_to_string(backend_type);
    
    // Add Azure Key Vault configuration if needed
    if (backend_type == SecretScopeBackendType::AZURE_KEYVAULT) {
        body_json["backend_azure_keyvault"] = {
            {"resource_id", azure_resource_id.value()},
            {"tenant_id", azure_tenant_id.value()},
            {"dns_name", dns_name.value()}
        };
    }
    
    std::string body = body_json.dump();
    internal::get_logger()->debug("Create scope request body: " + body);

    auto response = pimpl_->http_client_->post("/secrets/scopes/create", body);
    pimpl_->http_client_->check_response(response, "createScope");

    internal::get_logger()->info("Successfully created secret scope: " + scope);
}

void Secrets::delete_scope(const std::string& scope) {
    internal::get_logger()->info("Deleting secret scope: " + scope);

    // Build JSON body
    json body_json;
    body_json["scope"] = scope;
    std::string body = body_json.dump();

    internal::get_logger()->debug("Delete scope request body: " + body);

    // Make API request
    auto response = pimpl_->http_client_->post("/secrets/scopes/delete", body);
    pimpl_->http_client_->check_response(response, "deleteScope");

    internal::get_logger()->info("Successfully deleted secret scope: " + scope);
}

void Secrets::put_secret(const std::string& scope, const std::string& key, const std::string& value) {
    internal::get_logger()->info("Putting secret: scope=" + scope + ", key=" + key);

    // Build JSON body
    json body_json;
    body_json["scope"] = scope;
    body_json["key"] = key;
    body_json["string_value"] = value;
    std::string body = body_json.dump();

    // Make API request (DO NOT log the secret value!)
    internal::get_logger()->debug("Put secret request for scope=" + scope + ", key=" + key);
    auto response = pimpl_->http_client_->post("/secrets/put", body);
    pimpl_->http_client_->check_response(response, "putSecret");

    internal::get_logger()->info("Successfully put secret: scope=" + scope + ", key=" + key);
}

void Secrets::delete_secret(const std::string& scope, const std::string& key) {
    internal::get_logger()->info("Deleting secret: scope=" + scope + ", key=" + key);

    // Build JSON body
    json body_json;
    body_json["scope"] = scope;
    body_json["key"] = key;
    std::string body = body_json.dump();

    internal::get_logger()->debug("Delete secret request body: " + body);

    // Make API request
    auto response = pimpl_->http_client_->post("/secrets/delete", body);
    pimpl_->http_client_->check_response(response, "deleteSecret");

    internal::get_logger()->info("Successfully deleted secret: scope=" + scope + ", key=" + key);
}

std::vector<Secret> Secrets::list_secrets(const std::string& scope) {
    internal::get_logger()->info("Listing secrets in scope: " + scope);

    // Make GET request with scope as query parameter
    auto response = pimpl_->http_client_->get("/secrets/list?scope=" + scope);
    pimpl_->http_client_->check_response(response, "listSecrets");

    internal::get_logger()->debug("Successfully retrieved secrets list");
    return parse_secrets_list(response.body);
}

// ==================== FROM_JSON IMPLEMENTATIONS ====================

SecretScope SecretScope::from_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        SecretScope scope;

        scope.name = j.value("name", "");

        // Parse backend_type enum
        std::string backend_str = j.value("backend_type", "");
        if (backend_str == "DATABRICKS") {
            scope.backend_type = SecretScopeBackendType::DATABRICKS;
        } else if (backend_str == "AZURE_KEYVAULT") {
            scope.backend_type = SecretScopeBackendType::AZURE_KEYVAULT;
        } else {
            scope.backend_type = SecretScopeBackendType::UNKNOWN;
        }

        // Parse Azure Key Vault metadata if present
        if (j.contains("keyvault_metadata") && j["keyvault_metadata"].is_object()) {
            auto kv_meta = j["keyvault_metadata"];
            scope.resource_id = kv_meta.value("resource_id", "");
            scope.dns_name = kv_meta.value("dns_name", "");
        }

        return scope;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse SecretScope JSON: " + std::string(e.what()));
    }
}

Secret Secret::from_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        Secret secret;

        secret.key = j.value("key", "");
        secret.last_updated_timestamp = j.value("last_updated_timestamp", uint64_t(0));

        return secret;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse Secret JSON: " + std::string(e.what()));
    }
}

SecretACL SecretACL::from_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        SecretACL acl;

        acl.principal = j.value("principal", "");

        // Parse permission enum
        std::string perm_str = j.value("permission", "");
        if (perm_str == "READ") {
            acl.permission = SecretPermission::READ;
        } else if (perm_str == "WRITE") {
            acl.permission = SecretPermission::WRITE;
        } else if (perm_str == "MANAGE") {
            acl.permission = SecretPermission::MANAGE;
        } else {
            acl.permission = SecretPermission::UNKNOWN;
        }

        return acl;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse SecretACL JSON: " + std::string(e.what()));
    }
}

// ==================== PRIVATE HELPER METHODS ====================

std::string Secrets::secret_permissions_to_string(SecretPermission secret_permission) const {
    static const std::unordered_map<SecretPermission, std::string> permissions_map = {
        {SecretPermission::READ, "READ"},
        {SecretPermission::WRITE, "WRITE"},
        {SecretPermission::MANAGE, "MANAGE"},
        {SecretPermission::UNKNOWN, "UNKNOWN"},
    };

    auto it = permissions_map.find(secret_permission);
    if (it != permissions_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown SecretPermission");
}

std::string Secrets::backend_type_to_string(SecretScopeBackendType backend_type) const {
    static const std::unordered_map<SecretScopeBackendType, std::string> backend_map = {
        {SecretScopeBackendType::DATABRICKS, "DATABRICKS"},
        {SecretScopeBackendType::AZURE_KEYVAULT, "AZURE_KEYVAULT"}
    };

    auto it = backend_map.find(backend_type);
    if (it != backend_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown SecretScopeBackendType");
}

std::vector<SecretScope> Secrets::parse_scopes_list(const std::string& json_str) {
    std::vector<SecretScope> scopes;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("scopes") || !j["scopes"].is_array()) {
            internal::get_logger()->warn("No scopes array found in response");
            return scopes;
        }

        for (const auto& scope_json : j["scopes"]) {
            scopes.push_back(SecretScope::from_json(scope_json.dump()));
        }

        internal::get_logger()->info("Parsed " + std::to_string(scopes.size()) + " secret scopes");
    } catch (const json::exception& e) {
        internal::get_logger()->error("Failed to parse scopes list: " + std::string(e.what()));
        throw std::runtime_error("Failed to parse scopes list: " + std::string(e.what()));
    }

    return scopes;
}

std::vector<Secret> Secrets::parse_secrets_list(const std::string& json_str) {
    std::vector<Secret> secrets;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("secrets") || !j["secrets"].is_array()) {
            internal::get_logger()->warn("No secrets array found in response");
            return secrets;
        }

        for (const auto& secret_json : j["secrets"]) {
            secrets.push_back(Secret::from_json(secret_json.dump()));
        }

        internal::get_logger()->info("Parsed " + std::to_string(secrets.size()) + " secrets");
    } catch (const json::exception& e) {
        internal::get_logger()->error("Failed to parse secrets list: " + std::string(e.what()));
        throw std::runtime_error("Failed to parse secrets list: " + std::string(e.what()));
    }

    return secrets;
}

} // namespace databricks
