// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace databricks {

/**
 * @brief Enumeration of workspace object types
 */
enum class ObjectType {
    NOTEBOOK,  ///< Jupyter/Databricks notebook
    DIRECTORY, ///< Workspace directory/folder
    LIBRARY,   ///< Library (JAR, Python egg, etc.)
    FILE,      ///< Generic file
    REPO,      ///< Git repository
    UNKNOWN    ///< Unknown or unrecognized object type
};

/**
 * @brief Enumeration of export formats for workspace objects
 */
enum class ExportFormat {
    SOURCE,     ///< Export as source code (default)
    HTML,       ///< Export as HTML file
    JUPYTER,    ///< Export as Jupyter/IPython Notebook (.ipynb)
    DBC,        ///< Export as Databricks archive format
    R_MARKDOWN, ///< Export as R Markdown format
    AUTO        ///< Automatically detect format based on object type
};

/**
 * @brief Enumeration of import formats for workspace objects
 */
enum class ImportFormat {
    SOURCE,     ///< Import as source code
    HTML,       ///< Import as HTML file
    JUPYTER,    ///< Import as Jupyter/IPython Notebook (.ipynb)
    DBC,        ///< Import as Databricks archive format
    R_MARKDOWN, ///< Import as R Markdown format
    AUTO        ///< Automatically detect format (default)
};

/**
 * @brief Enumeration of notebook programming languages
 */
enum class Language {
    SCALA,  ///< Scala programming language
    PYTHON, ///< Python programming language
    SQL,    ///< SQL query language
    R,      ///< R programming language
    UNKNOWN ///< Unknown or unrecognized language
};

/**
 * @brief Represents metadata for a workspace object (file, notebook, or directory)
 */
struct ObjectInfo {
    std::string path;                       ///< Absolute workspace path (e.g., "/Users/user@example.com/notebook")
    ObjectType object_type = ObjectType::UNKNOWN; ///< Type of the object
    uint64_t object_id = 0;                 ///< Unique numerical identifier for the object
    Language language = Language::UNKNOWN;  ///< Language (only set for NOTEBOOK type)
    uint64_t size = 0;                      ///< Size in bytes (for files)
    uint64_t created_at = 0;                ///< Creation timestamp (Unix milliseconds)
    uint64_t modified_at = 0;               ///< Last modification timestamp (Unix milliseconds)

    /**
     * @brief Parse an ObjectInfo from JSON string
     * @param json_str JSON representation of a workspace object
     * @return Parsed ObjectInfo object
     * @throws std::runtime_error if parsing fails
     */
    static ObjectInfo from_json(const std::string& json_str);
};

/**
 * @brief Response from the export endpoint containing exported content
 */
struct ExportResponse {
    std::string content;      ///< Base64-encoded content of the exported object
    std::string file_type;    ///< File type indicator (e.g., "py", "scala", "sql")

    /**
     * @brief Parse an ExportResponse from JSON string
     * @param json_str JSON representation of an export response
     * @return Parsed ExportResponse object
     * @throws std::runtime_error if parsing fails
     */
    static ExportResponse from_json(const std::string& json_str);
};

/**
 * @brief Request parameters for importing a workspace object
 */
struct ImportRequest {
    std::string path;                           ///< Absolute workspace path for the imported object
    std::string content;                        ///< Base64-encoded content to import
    ImportFormat format = ImportFormat::AUTO;   ///< Format of the content being imported
    Language language = Language::UNKNOWN;      ///< Language (required for SOURCE format, single file)
    bool overwrite = false;                     ///< Whether to overwrite existing object
};

} // namespace databricks
