// Copyright (c) 2026 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace databricks {
namespace internal {

/**
 * @brief Validate and sanitize a file path to prevent path traversal attacks
 *
 * This function validates that a path:
 * - Does not contain path traversal sequences like "../" or ".."
 * - Does not contain null bytes
 * - Does not start with unexpected absolute paths
 * - Is within reasonable length limits
 *
 * @param path The file path to validate
 * @param allow_absolute If true, allows absolute paths; if false, only relative paths
 * @return std::string The validated path (same as input if valid)
 * @throws std::invalid_argument if the path contains suspicious patterns
 *
 * @example
 * std::string safe = validate_path("/home/user/.databrickscfg", true);
 * // Throws if path contains ".." or other suspicious patterns
 */
std::string validate_path(const std::string& path, bool allow_absolute = true);

/**
 * @brief Safely construct a path by joining base directory with filename
 *
 * Validates both components and ensures the result does not escape the base directory.
 * This is the recommended way to construct file paths from user input.
 *
 * @param base_dir The base directory (e.g., from HOME environment variable)
 * @param filename The filename to append (e.g., ".databrickscfg")
 * @return std::string The safely constructed path
 * @throws std::invalid_argument if either component is invalid or result escapes base
 *
 * @example
 * const char* home = std::getenv("HOME");
 * std::string config_path = safe_join_path(home, ".databrickscfg");
 */
std::string safe_join_path(const std::string& base_dir, const std::string& filename);

/**
 * @brief Validate a path from an environment variable
 *
 * Performs additional validation specific to paths from environment variables,
 * including checks for suspicious patterns and reasonable path lengths.
 *
 * @param path The path from the environment variable
 * @param env_var_name Name of the environment variable (for error messages)
 * @return std::string The validated path
 * @throws std::invalid_argument if the path is invalid or suspicious
 *
 * @example
 * const char* log_file = std::getenv("DATABRICKS_LOG_FILE");
 * if (log_file) {
 *     std::string safe_log = validate_env_path(log_file, "DATABRICKS_LOG_FILE");
 * }
 */
std::string validate_env_path(const std::string& path, const std::string& env_var_name);

} // namespace internal
} // namespace databricks
