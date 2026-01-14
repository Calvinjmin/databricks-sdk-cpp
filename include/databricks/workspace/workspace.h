// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include "databricks/core/config.h"
#include "databricks/workspace/workspace_types.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace databricks {
namespace internal {
class IHttpClient;
} // namespace internal

/**
 * @brief Client for interacting with the Databricks Workspace API
 *
 * The Workspace API allows you to manage workspace objects such as notebooks, directories,
 * and files in your Databricks workspace. You can list, import, export, delete, and create
 * directories programmatically. This implementation uses Workspace API 2.0.
 *
 * Example usage:
 * @code
 * databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
 * databricks::Workspace workspace(auth);
 *
 * // List contents of a directory
 * auto objects = workspace.list("/Users/user@example.com");
 * for (const auto& obj : objects) {
 *     std::cout << obj.path << " (" << obj.object_type << ")" << std::endl;
 * }
 *
 * // Create a directory
 * workspace.mkdirs("/Users/user@example.com/my_project");
 *
 * // Export a notebook
 * auto exported = workspace.export_file("/Users/user@example.com/notebook",
 *                                        databricks::ExportFormat::SOURCE);
 *
 * // Get object status
 * auto info = workspace.get_status("/Users/user@example.com/notebook");
 *
 * // Delete an object
 * workspace.delete_object("/Users/user@example.com/old_notebook", false);
 * @endcode
 */
class Workspace {
public:
    /**
     * @brief Construct a Workspace API client
     * @param auth Authentication configuration with host and token
     * @param api_version Workspace API version to use (default: "2.0")
     */
    explicit Workspace(const AuthConfig& auth, const std::string& api_version = "2.0");

    /**
     * @brief Construct a Workspace API client with dependency injection (for testing)
     * @param http_client Injected HTTP client (use MockHttpClient for unit tests)
     * @note This constructor is primarily for testing with mock HTTP clients
     */
    explicit Workspace(std::shared_ptr<internal::IHttpClient> http_client);

    /**
     * @brief Destructor
     */
    ~Workspace();

    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;

    // Directory operations

    /**
     * @brief List the contents of a directory or get information about a single object
     *
     * @param path The absolute workspace path of the notebook or directory
     * @param notebooks_modified_after Optional filter to only return notebooks modified after
     *                                  this Unix timestamp (in milliseconds)
     * @return Vector of ObjectInfo representing the contents (empty if path is a file)
     * @throws std::runtime_error if the path does not exist (RESOURCE_DOES_NOT_EXIST)
     *
     * @note If the path points to a notebook or file (not a directory), this returns
     *       a vector with a single ObjectInfo element describing that object.
     */
    std::vector<ObjectInfo> list(const std::string& path,
                                 const std::optional<uint64_t>& notebooks_modified_after = std::nullopt);

    /**
     * @brief Create the specified directory and necessary parent directories if they don't exist
     *
     * @param path The absolute workspace path of the directory to create
     * @throws std::runtime_error if there is an object (not a directory) at any prefix
     *                            of the input path (RESOURCE_ALREADY_EXISTS)
     *
     * @note This operation is idempotent - if the directory already exists, it succeeds.
     *       If this operation fails, some parent directories may have been created.
     */
    void mkdirs(const std::string& path);

    /**
     * @brief Get the status/metadata of a workspace object or directory
     *
     * @param path The absolute workspace path of the notebook or directory
     * @return ObjectInfo containing metadata about the object
     * @throws std::runtime_error if the path does not exist (RESOURCE_DOES_NOT_EXIST)
     */
    ObjectInfo get_status(const std::string& path);

    // Export operations

    /**
     * @brief Export a notebook or directory from the workspace
     *
     * @param path The absolute workspace path of the object or directory to export
     * @param format The format for the exported content (default: SOURCE)
     * @return ExportResponse containing the base64-encoded exported content
     * @throws std::runtime_error if the path does not exist (RESOURCE_DOES_NOT_EXIST)
     * @throws std::runtime_error if the exported data exceeds size limit (MAX_NOTEBOOK_SIZE_EXCEEDED)
     *
     * @note Exporting a directory is only supported for DBC, SOURCE, and AUTO formats.
     *       The content field in the response is base64-encoded.
     */
    ExportResponse export_file(const std::string& path, ExportFormat format = ExportFormat::SOURCE);

    // Import operations

    /**
     * @brief Import a notebook or file into the workspace
     *
     * @param path The absolute workspace path where the object should be imported
     * @param content Base64-encoded content to import
     * @param format The format of the content being imported (default: AUTO)
     * @param language The language of the notebook (required for SOURCE format with single files)
     * @param overwrite Whether to overwrite the object if it already exists (default: false)
     * @throws std::runtime_error if path already exists and overwrite is false
     *                            (RESOURCE_ALREADY_EXISTS)
     *
     * @note To import a directory, use DBC or SOURCE format with language unset.
     *       To import a single file as SOURCE, you must set the language field.
     */
    void import_file(const std::string& path, const std::string& content, ImportFormat format = ImportFormat::AUTO,
                     const std::optional<Language>& language = std::nullopt, bool overwrite = false);

    /**
     * @brief Import a notebook or file into the workspace using an ImportRequest struct
     *
     * @param request ImportRequest containing all import parameters
     * @throws std::runtime_error if path already exists and overwrite is false
     *                            (RESOURCE_ALREADY_EXISTS)
     *
     * @note This is a convenience overload that accepts an ImportRequest struct
     */
    void import_file(const ImportRequest& request);

    // Delete operations

    /**
     * @brief Delete a workspace object or directory
     *
     * @param path The absolute workspace path of the notebook or directory to delete
     * @param recursive Whether to recursively delete all objects in a directory (default: false)
     * @throws std::runtime_error if the path does not exist (RESOURCE_DOES_NOT_EXIST)
     * @throws std::runtime_error if path is a non-empty directory and recursive is false
     *                            (DIRECTORY_NOT_EMPTY)
     *
     * @note Deleted objects do not go to the Trash folder and cannot be recovered.
     *       Use with caution!
     */
    void delete_object(const std::string& path, bool recursive = false);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;

    // Helper methods for enum conversions
    std::string object_type_to_string(ObjectType type) const;
    std::string export_format_to_string(ExportFormat format) const;
    std::string import_format_to_string(ImportFormat format) const;
    std::string language_to_string(Language language) const;

    static ObjectType string_to_object_type(const std::string& str);
    static ExportFormat string_to_export_format(const std::string& str);
    static ImportFormat string_to_import_format(const std::string& str);
    static Language string_to_language(const std::string& str);

    // Helper methods for parsing API responses
    static std::vector<ObjectInfo> parse_list_response(const std::string& json_str);
    static ObjectInfo parse_object_info(const std::string& json_str);
};

} // namespace databricks
