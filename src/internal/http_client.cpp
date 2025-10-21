#include "http_client.h"
#include "logger.h"

#include <curl/curl.h>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <chrono>

namespace databricks {
    namespace internal {
        // Callback for libcurl to write response body
        static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
            size_t total_size = size * nmemb;
            std::string* response_str = static_cast<std::string*>(userp);
            response_str->append(static_cast<char*>(contents), total_size);
            return total_size;
        }

        // Callback for libcurl to capture response headers
        static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
            size_t total_size = size * nitems;
            auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);

            std::string header(buffer, total_size);

            // Parse "Key: Value" format
            size_t colon_pos = header.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = header.substr(0, colon_pos);
                std::string value = header.substr(colon_pos + 1);

                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);

                (*headers)[key] = value;
            }

            return total_size;
        }

        HttpClient::HttpClient(const AuthConfig& auth, const std::string& api_version)
            : auth_(auth), api_version_(api_version) {
            static bool curl_initialized = false;

            // Initialize Curl Client
            if (!curl_initialized) {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                curl_initialized = true;
            }
        }

        std::string HttpClient::get_base_url() const {
            return auth_.host + "/api/" + api_version_;
        }

        std::map<std::string, std::string> HttpClient::get_headers() const {
            return {
                {"Authorization", "Bearer " + auth_.token},
                {"Content-Type", "application/json"},
                {"Accept", "application/json"}
            };
        }

        // ============================================================================
        // Retry Helper Methods
        // ============================================================================

        bool HttpClient::should_retry(int status_code, int attempt) const {
            const int MAX_RETRIES = 3;

            if (attempt >= MAX_RETRIES) {
                return false;
            }

            // Retry on these HTTP status codes
            return status_code == 408 ||  // Request Timeout
                   status_code == 429 ||  // Too Many Requests (rate limit)
                   status_code == 500 ||  // Internal Server Error
                   status_code == 502 ||  // Bad Gateway
                   status_code == 503 ||  // Service Unavailable
                   status_code == 504;    // Gateway Timeout
        }

        int HttpClient::calculate_backoff(int attempt) const {
            const int INITIAL_BACKOFF_MS = 1000;  // 1 second

            // Exponential backoff: 1s, 2s, 4s, 8s, etc.
            return INITIAL_BACKOFF_MS * (1 << attempt);
        }

        // ============================================================================
        // Core HTTP Execution Methods (without retry)
        // ============================================================================

        HttpResponse HttpClient::execute_get(const std::string& path) {
            CURL* curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("Failed to initialize CURL");
            }

            HttpResponse response;
            std::string url = get_base_url() + path;

            internal::get_logger()->debug("HTTP GET: " + url);

            // Set CURL options
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, auth_.timeout_seconds);

            // Set headers
            struct curl_slist* headers = nullptr;
            for (const auto& [key, value] : get_headers()) {
                std::string header = key + ": " + value;
                headers = curl_slist_append(headers, header.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Perform request
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::string error_msg = "CURL request failed: " + std::string(curl_easy_strerror(res));
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                internal::get_logger()->error(error_msg);
                throw std::runtime_error(error_msg);
            }

            // Get HTTP status code
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            response.status_code = static_cast<int>(http_code);

            // Cleanup
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            internal::get_logger()->debug("HTTP Response: " + std::to_string(response.status_code));

            return response;
        }

        HttpResponse HttpClient::execute_post(const std::string& path, const std::string& json_body) {
            CURL* curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("Failed to initialize CURL");
            }

            HttpResponse response;
            std::string url = get_base_url() + path;

            internal::get_logger()->debug("HTTP POST: " + url);
            internal::get_logger()->debug("Body: " + json_body);

            // Set CURL options
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, auth_.timeout_seconds);

            // Set headers
            struct curl_slist* headers = nullptr;
            for (const auto& [key, value] : get_headers()) {
                std::string header = key + ": " + value;
                headers = curl_slist_append(headers, header.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Perform request
            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::string error_msg = "CURL request failed: " + std::string(curl_easy_strerror(res));
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                internal::get_logger()->error(error_msg);
                throw std::runtime_error(error_msg);
            }

            // Get HTTP status code
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            response.status_code = static_cast<int>(http_code);

            // Cleanup
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            internal::get_logger()->debug("HTTP Response: " + std::to_string(response.status_code));

            return response;
        }

        // ============================================================================
        // Public HTTP Methods (with retry)
        // ============================================================================

        HttpResponse HttpClient::get(const std::string& path) {
            const int MAX_RETRIES = 3;
            HttpResponse response;

            for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
                try {
                    response = execute_get(path);

                    // Success - return immediately
                    if (response.status_code == 200) {
                        return response;
                    }

                    // Check if we should retry
                    if (!should_retry(response.status_code, attempt + 1)) {
                        // Non-retryable error or max retries reached
                        return response;
                    }

                    // Calculate backoff and log retry
                    int backoff_ms = calculate_backoff(attempt);
                    internal::get_logger()->warn(
                        "GET request failed with HTTP " + std::to_string(response.status_code) +
                        ". Retrying in " + std::to_string(backoff_ms) + "ms " +
                        "(attempt " + std::to_string(attempt + 2) + "/" + std::to_string(MAX_RETRIES) + ")"
                    );

                    // Wait before retry
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));

                } catch (const std::runtime_error& e) {
                    // Connection error - retry if not last attempt
                    if (attempt + 1 >= MAX_RETRIES) {
                        throw;  // Re-throw on last attempt
                    }

                    int backoff_ms = calculate_backoff(attempt);
                    internal::get_logger()->warn(
                        "GET connection error: " + std::string(e.what()) +
                        ". Retrying in " + std::to_string(backoff_ms) + "ms"
                    );

                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                }
            }

            // Return last response if all retries exhausted
            return response;
        }

        HttpResponse HttpClient::post(const std::string& path, const std::string& json_body) {
            const int MAX_RETRIES = 3;
            HttpResponse response;

            for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
                try {
                    response = execute_post(path, json_body);

                    // Success - return immediately
                    if (response.status_code == 200) {
                        return response;
                    }

                    // Check if we should retry
                    if (!should_retry(response.status_code, attempt + 1)) {
                        // Non-retryable error or max retries reached
                        return response;
                    }

                    // Calculate backoff and log retry
                    int backoff_ms = calculate_backoff(attempt);
                    internal::get_logger()->warn(
                        "POST request failed with HTTP " + std::to_string(response.status_code) +
                        ". Retrying in " + std::to_string(backoff_ms) + "ms " +
                        "(attempt " + std::to_string(attempt + 2) + "/" + std::to_string(MAX_RETRIES) + ")"
                    );

                    // Wait before retry
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));

                } catch (const std::runtime_error& e) {
                    // Connection error - retry if not last attempt
                    if (attempt + 1 >= MAX_RETRIES) {
                        throw;  // Re-throw on last attempt
                    }

                    int backoff_ms = calculate_backoff(attempt);
                    internal::get_logger()->warn(
                        "POST connection error: " + std::string(e.what()) +
                        ". Retrying in " + std::to_string(backoff_ms) + "ms"
                    );

                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                }
            }

            // Return last response if all retries exhausted
            return response;
        }

        void HttpClient::check_response(const HttpResponse& response, const std::string& operation_name ) const {
            if (response.status_code != 200) {
                std::string error_msg = "Failed to " + operation_name + ": HTTP " +
                                        std::to_string(response.status_code) +
                                        " - " + response.body;
                internal::get_logger()->error(error_msg);
                throw std::runtime_error(error_msg);
            }
        }
    }
}