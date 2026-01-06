// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#include "databricks/workspace/workspace.h"

#include "../internal/http_client.h"
#include "../internal/http_client_interface.h"
#include "../internal/logger.h"
#include "../internal/url_utils.h"

#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace databricks {

// ==================== PIMPL IMPLEMENTATION ====================

class Workspace::Impl {
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

Workspace::Workspace(const AuthConfig& auth, const std::string& api_version)
    : pimpl_(std::make_unique<Impl>(auth, api_version)) {}

Workspace::Workspace(std::shared_ptr<internal::IHttpClient> http_client)
    : pimpl_(std::make_unique<Impl>(std::move(http_client))) {}

Workspace::~Workspace() = default;

// ==================== PUBLIC API METHODS ====================

std::vector<ObjectInfo> Workspace::list(const std::string& path,
                                         const std::optional<uint64_t>& notebooks_modified_after) {
    internal::get_logger()->info("Listing workspace objects at path: " + path);

    // Throw exception if path is empty
    if (path.length() == 0) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Build query parameters with URL encoding
    std::string query_params = "?path=" + internal::url_encode(path);
    if (notebooks_modified_after.has_value()) {
        query_params += "&notebooks_modified_after=" + std::to_string(notebooks_modified_after.value());
    }

    auto response = pimpl_->http_client_->get("/workspace/list" + query_params);
    pimpl_->http_client_->check_response(response, "list");

    internal::get_logger()->debug("Successfully retrieved workspace list");
    return parse_list_response(response.body);
}

void Workspace::mkdirs(const std::string& path) {
    internal::get_logger()->info("Creating workspace directory: " + path);

    // Throw exception if path is empty
    if (path.length() == 0) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Build JSON body
    json body_json;
    body_json["path"] = path;
    std::string body = body_json.dump();

    internal::get_logger()->debug("Mkdirs request body: " + body);

    auto response = pimpl_->http_client_->post("/workspace/mkdirs", body);
    pimpl_->http_client_->check_response(response, "mkdirs");

    internal::get_logger()->info("Successfully created workspace directory: " + path);
}

ObjectInfo Workspace::get_status(const std::string& path) {
    internal::get_logger()->info("Getting status for workspace object: " + path);

    // Throw exception if path is empty
    if (path.length() == 0) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Build query parameters with URL encoding
    std::string query_params = "?path=" + internal::url_encode(path);

    auto response = pimpl_->http_client_->get("/workspace/get-status" + query_params);
    pimpl_->http_client_->check_response(response, "getStatus");

    internal::get_logger()->debug("Successfully retrieved object status");
    return parse_object_info(response.body);
}

ExportResponse Workspace::export_file(const std::string& path, ExportFormat format) {
    internal::get_logger()->info("Exporting workspace object: " + path);

    // Throw exception if path is empty
    if (path.length() == 0) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Build query parameters with URL encoding
    std::string query_params = "?path=" + internal::url_encode(path);
    query_params += "&format=" + export_format_to_string(format);

    internal::get_logger()->debug("Export request for path=" + path + ", format=" + export_format_to_string(format));

    auto response = pimpl_->http_client_->get("/workspace/export" + query_params);
    pimpl_->http_client_->check_response(response, "export");

    internal::get_logger()->info("Successfully exported workspace object: " + path);
    return ExportResponse::from_json(response.body);
}

void Workspace::import_file(const std::string& path,
                             const std::string& content,
                             ImportFormat format,
                             const std::optional<Language>& language,
                             bool overwrite) {
    internal::get_logger()->info("Importing workspace object to path: " + path);

    // Throw exception for invalid arguments
    if (path.length() == 0 || content.length() == 0 ) {
        throw std::invalid_argument("Path or Content Input cannot be empty");
    }

    // Build JSON body
    json body_json;
    body_json["path"] = path;
    body_json["content"] = content;
    body_json["format"] = import_format_to_string(format);
    body_json["overwrite"] = overwrite;

    // Add language if specified (required for SOURCE format with single files)
    if (language.has_value()) {
        body_json["language"] = language_to_string(language.value());
    }

    std::string body = body_json.dump();
    internal::get_logger()->debug("Import request for path=" + path + ", format=" + import_format_to_string(format));

    auto response = pimpl_->http_client_->post("/workspace/import", body);
    pimpl_->http_client_->check_response(response, "import");

    internal::get_logger()->info("Successfully imported workspace object: " + path);
}

void Workspace::import_file(const ImportRequest& request) {
    std::optional<Language> lang = std::nullopt;
    if (request.language != Language::UNKNOWN) {
        lang = request.language;
    }

    import_file(request.path, request.content, request.format, lang, request.overwrite);
}

void Workspace::delete_object(const std::string& path, bool recursive) {
    internal::get_logger()->info("Deleting workspace object: " + path + " (recursive=" + (recursive ? "true" : "false") + ")");

    // Throw exception if path is empty
    if (path.length() == 0) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Build JSON body
    json body_json;
    body_json["path"] = path;
    body_json["recursive"] = recursive;
    std::string body = body_json.dump();

    internal::get_logger()->debug("Delete request body: " + body);

    auto response = pimpl_->http_client_->post("/workspace/delete", body);
    pimpl_->http_client_->check_response(response, "delete");

    internal::get_logger()->info("Successfully deleted workspace object: " + path);
}

// ==================== ENUM CONVERSION HELPERS ====================

std::string Workspace::object_type_to_string(ObjectType type) const {
    static const std::unordered_map<ObjectType, std::string> type_map = {
        {ObjectType::NOTEBOOK, "NOTEBOOK"},
        {ObjectType::DIRECTORY, "DIRECTORY"},
        {ObjectType::LIBRARY, "LIBRARY"},
        {ObjectType::FILE, "FILE"},
        {ObjectType::REPO, "REPO"}
    };

    auto it = type_map.find(type);
    if (it != type_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown ObjectType");
}

ObjectType Workspace::string_to_object_type(const std::string& str) {
    static const std::unordered_map<std::string, ObjectType> type_map = {
        {"NOTEBOOK", ObjectType::NOTEBOOK},
        {"DIRECTORY", ObjectType::DIRECTORY},
        {"LIBRARY", ObjectType::LIBRARY},
        {"FILE", ObjectType::FILE},
        {"REPO", ObjectType::REPO}
    };

    auto it = type_map.find(str);
    if (it != type_map.end()) {
        return it->second;
    }
    return ObjectType::UNKNOWN;
}

std::string Workspace::export_format_to_string(ExportFormat format) const {
    static const std::unordered_map<ExportFormat, std::string> format_map = {
        {ExportFormat::SOURCE, "SOURCE"},
        {ExportFormat::HTML, "HTML"},
        {ExportFormat::JUPYTER, "JUPYTER"},
        {ExportFormat::DBC, "DBC"},
        {ExportFormat::R_MARKDOWN, "R_MARKDOWN"},
        {ExportFormat::AUTO, "AUTO"}
    };

    auto it = format_map.find(format);
    if (it != format_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown ExportFormat");
}

ExportFormat Workspace::string_to_export_format(const std::string& str) {
    static const std::unordered_map<std::string, ExportFormat> format_map = {
        {"SOURCE", ExportFormat::SOURCE},
        {"HTML", ExportFormat::HTML},
        {"JUPYTER", ExportFormat::JUPYTER},
        {"DBC", ExportFormat::DBC},
        {"R_MARKDOWN", ExportFormat::R_MARKDOWN},
        {"AUTO", ExportFormat::AUTO}
    };

    auto it = format_map.find(str);
    if (it != format_map.end()) {
        return it->second;
    }
    return ExportFormat::SOURCE; // Default
}

std::string Workspace::import_format_to_string(ImportFormat format) const {
    static const std::unordered_map<ImportFormat, std::string> format_map = {
        {ImportFormat::SOURCE, "SOURCE"},
        {ImportFormat::HTML, "HTML"},
        {ImportFormat::JUPYTER, "JUPYTER"},
        {ImportFormat::DBC, "DBC"},
        {ImportFormat::R_MARKDOWN, "R_MARKDOWN"},
        {ImportFormat::AUTO, "AUTO"}
    };

    auto it = format_map.find(format);
    if (it != format_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown ImportFormat");
}

ImportFormat Workspace::string_to_import_format(const std::string& str) {
    static const std::unordered_map<std::string, ImportFormat> format_map = {
        {"SOURCE", ImportFormat::SOURCE},
        {"HTML", ImportFormat::HTML},
        {"JUPYTER", ImportFormat::JUPYTER},
        {"DBC", ImportFormat::DBC},
        {"R_MARKDOWN", ImportFormat::R_MARKDOWN},
        {"AUTO", ImportFormat::AUTO}
    };

    auto it = format_map.find(str);
    if (it != format_map.end()) {
        return it->second;
    }
    return ImportFormat::AUTO; // Default
}

std::string Workspace::language_to_string(Language language) const {
    static const std::unordered_map<Language, std::string> lang_map = {
        {Language::SCALA, "SCALA"},
        {Language::PYTHON, "PYTHON"},
        {Language::SQL, "SQL"},
        {Language::R, "R"}
    };

    auto it = lang_map.find(language);
    if (it != lang_map.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown Language");
}

Language Workspace::string_to_language(const std::string& str) {
    static const std::unordered_map<std::string, Language> lang_map = {
        {"SCALA", Language::SCALA},
        {"PYTHON", Language::PYTHON},
        {"SQL", Language::SQL},
        {"R", Language::R}
    };

    auto it = lang_map.find(str);
    if (it != lang_map.end()) {
        return it->second;
    }
    return Language::UNKNOWN;
}

// ==================== FROM_JSON IMPLEMENTATIONS ====================

ObjectInfo ObjectInfo::from_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        ObjectInfo info;

        info.path = j.value("path", "");
        info.object_id = j.value("object_id", uint64_t(0));

        // Parse object_type enum
        std::string type_str = j.value("object_type", "");
        if (type_str == "NOTEBOOK") {
            info.object_type = ObjectType::NOTEBOOK;
        } else if (type_str == "DIRECTORY") {
            info.object_type = ObjectType::DIRECTORY;
        } else if (type_str == "LIBRARY") {
            info.object_type = ObjectType::LIBRARY;
        } else if (type_str == "FILE") {
            info.object_type = ObjectType::FILE;
        } else if (type_str == "REPO") {
            info.object_type = ObjectType::REPO;
        } else {
            info.object_type = ObjectType::UNKNOWN;
        }

        // Parse language enum (only present for NOTEBOOK type)
        if (j.contains("language")) {
            std::string lang_str = j.value("language", "");
            if (lang_str == "SCALA") {
                info.language = Language::SCALA;
            } else if (lang_str == "PYTHON") {
                info.language = Language::PYTHON;
            } else if (lang_str == "SQL") {
                info.language = Language::SQL;
            } else if (lang_str == "R") {
                info.language = Language::R;
            } else {
                info.language = Language::UNKNOWN;
            }
        }

        // Optional fields
        info.size = j.value("size", uint64_t(0));
        info.created_at = j.value("created_at", uint64_t(0));
        info.modified_at = j.value("modified_at", uint64_t(0));

        return info;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse ObjectInfo JSON: " + std::string(e.what()));
    }
}

ExportResponse ExportResponse::from_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        ExportResponse response;

        response.content = j.value("content", "");
        response.file_type = j.value("file_type", "");

        return response;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse ExportResponse JSON: " + std::string(e.what()));
    }
}

// ==================== PRIVATE HELPER METHODS ====================

std::vector<ObjectInfo> Workspace::parse_list_response(const std::string& json_str) {
    std::vector<ObjectInfo> objects;

    try {
        auto j = json::parse(json_str);

        if (!j.contains("objects") || !j["objects"].is_array()) {
            internal::get_logger()->warn("No objects array found in list response");
            return objects;
        }

        for (const auto& obj_json : j["objects"]) {
            objects.push_back(ObjectInfo::from_json(obj_json.dump()));
        }

        internal::get_logger()->info("Parsed " + std::to_string(objects.size()) + " workspace objects");
    } catch (const json::exception& e) {
        internal::get_logger()->error("Failed to parse list response: " + std::string(e.what()));
        throw std::runtime_error("Failed to parse list response: " + std::string(e.what()));
    }

    return objects;
}

ObjectInfo Workspace::parse_object_info(const std::string& json_str) {
    try {
        return ObjectInfo::from_json(json_str);
    } catch (const std::runtime_error& e) {
        internal::get_logger()->error("Failed to parse object info: " + std::string(e.what()));
        throw;
    }
}

} // namespace databricks
