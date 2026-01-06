// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include "url_utils.h"

#include <stdexcept>

#include <curl/curl.h>

namespace databricks {
namespace internal {

std::string url_encode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL for URL encoding");
    }

    char* encoded = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
    if (!encoded) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to URL encode string");
    }

    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}

} // namespace internal
} // namespace databricks
