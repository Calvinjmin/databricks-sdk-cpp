#pragma once

#include "databricks/core/config.h"

#include "http_client_interface.h"

#include <map>
#include <string>

namespace databricks {
namespace internal {
/**
 * @brief Internal HTTP Client Wrapper around libcurl
 *
 * Production implementation of IHttpClient that makes real HTTP requests.
 */
class HttpClient : public IHttpClient {
public:
    explicit HttpClient(const AuthConfig& auth, const std::string& api_version = "2.2");

    /**
     * @brief Wrapper around a GET REST API Call
     *
     * @param path URL Path to Get Request
     *
     * @return HttpResponse HTTP Response Object
     */
    HttpResponse get(const std::string& path) override;

    /**
     * @brief Wrapper around a POST REST API Call
     *
     * @param path URL Path to Post Request
     * @param json_body Request Payload
     *
     * @return HttpResponse HTTP Response Object
     */
    HttpResponse post(const std::string& path, const std::string& json_body) override;

    void check_response(const HttpResponse& response, const std::string& operation_name) const override;

private:
    AuthConfig auth_;
    std::string api_version_;
    std::string get_base_url() const;
    std::map<std::string, std::string> get_headers() const;

    // Retry helper methods
    bool should_retry(int status_code, int attempt) const;
    int calculate_backoff(int attempt) const;

    // Core HTTP execution methods
    HttpResponse execute_get(const std::string& path);
    HttpResponse execute_post(const std::string& path, const std::string& json_body);
};
} // namespace internal
} // namespace databricks