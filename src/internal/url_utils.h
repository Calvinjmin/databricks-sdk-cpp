// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace databricks {
namespace internal {

/**
 * @brief URL-encode a string to safely include in query parameters
 *
 * Encodes special characters in a string to make it safe for use in URLs.
 * Uses libcurl's curl_easy_escape function for RFC 3986 compliant encoding.
 *
 * @param str String to encode
 * @return std::string URL-encoded string
 *
 * @throws std::runtime_error if CURL initialization or encoding fails
 *
 * @example
 * std::string encoded = url_encode("hello world");
 * // Returns: "hello%20world"
 *
 * std::string safe = url_encode("cluster&id=123");
 * // Returns: "cluster%26id%3D123"
 */
std::string url_encode(const std::string& str);

} // namespace internal
} // namespace databricks
