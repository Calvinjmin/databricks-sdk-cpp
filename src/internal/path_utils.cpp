// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#include "path_utils.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace databricks {
namespace internal {

std::string validate_path(const std::string& path, bool allow_absolute) {
    // Check for empty path
    if (path.empty()) {
        throw std::invalid_argument("Path cannot be empty");
    }

    // Check for null bytes (potential injection)
    if (path.find('\0') != std::string::npos) {
        throw std::invalid_argument("Path contains null byte");
    }

    // Check for excessively long paths (potential DoS)
    const size_t MAX_PATH_LENGTH = 4096;
    if (path.length() > MAX_PATH_LENGTH) {
        throw std::invalid_argument("Path exceeds maximum length of " + std::to_string(MAX_PATH_LENGTH));
    }

    // Check for path traversal patterns
    // Look for ".." as a complete path component (not part of a filename)
    size_t pos = 0;
    while ((pos = path.find("..", pos)) != std::string::npos) {
        // Check if ".." is a standalone component
        bool is_start = (pos == 0);
        bool is_end = (pos + 2 >= path.length());
        bool before_separator = (pos > 0 && path[pos - 1] == '/');
        bool after_separator = (pos + 2 < path.length() && path[pos + 2] == '/');

        if ((is_start || before_separator) && (is_end || after_separator)) {
            throw std::invalid_argument("Path contains path traversal sequence (..)");
        }
        pos++;
    }

    // Check for absolute paths if not allowed
    if (!allow_absolute && !path.empty() && path[0] == '/') {
        throw std::invalid_argument("Absolute paths are not allowed");
    }

    // Additional security checks for suspicious patterns
    if (path.find("/../") != std::string::npos || path.find("/./") != std::string::npos) {
        throw std::invalid_argument("Path contains suspicious pattern");
    }

    // Check for paths starting with ".."
    if (path.find("..") == 0 && (path.length() == 2 || path[2] == '/')) {
        throw std::invalid_argument("Path starts with path traversal sequence");
    }

    // Check for paths ending with "/.."
    if (path.length() >= 3 && path.substr(path.length() - 3) == "/..") {
        throw std::invalid_argument("Path ends with path traversal sequence");
    }

    return path;
}

std::string safe_join_path(const std::string& base_dir, const std::string& filename) {
    // Validate base directory
    if (base_dir.empty()) {
        throw std::invalid_argument("Base directory cannot be empty");
    }

    validate_path(base_dir, true);

    // Validate filename
    if (filename.empty()) {
        throw std::invalid_argument("Filename cannot be empty");
    }

    // Filename should not contain directory separators or path traversal
    if (filename.find('/') != std::string::npos) {
        throw std::invalid_argument("Filename cannot contain directory separators");
    }

    if (filename.find("..") != std::string::npos) {
        throw std::invalid_argument("Filename cannot contain path traversal sequence");
    }

    // Check for null bytes
    if (filename.find('\0') != std::string::npos) {
        throw std::invalid_argument("Filename contains null byte");
    }

    // Construct the path
    std::string result = base_dir;
    if (!result.empty() && result.back() != '/') {
        result += '/';
    }
    result += filename;

    // Final validation of the complete path
    validate_path(result, true);

    return result;
}

std::string validate_env_path(const std::string& path, const std::string& env_var_name) {
    // Additional validation for environment variable paths
    if (path.empty()) {
        throw std::invalid_argument("Environment variable " + env_var_name + " is empty");
    }

    // Check for control characters (potential injection)
    for (char c : path) {
        if (std::iscntrl(static_cast<unsigned char>(c)) && c != '\0') {
            throw std::invalid_argument("Path from " + env_var_name + " contains control characters");
        }
    }

    // Validate the path using standard validation
    validate_path(path, true);

    // Additional check: ensure path doesn't start with suspicious patterns
    const std::string dangerous_prefixes[] = {"/etc/", "/sys/", "/proc/", "/dev/"};

    for (const auto& prefix : dangerous_prefixes) {
        if (path.find(prefix) == 0) {
            throw std::invalid_argument("Path from " + env_var_name +
                                        " points to restricted system directory: " + prefix);
        }
    }

    return path;
}

} // namespace internal
} // namespace databricks
