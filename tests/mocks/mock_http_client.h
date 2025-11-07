#pragma once

#include "../../src/internal/http_client_interface.h"

#include <gmock/gmock.h>

namespace databricks {
namespace test {

/**
 * @brief Mock implementation of IHttpClient for unit testing
 *
 * This mock allows tests to simulate HTTP responses without making real network calls.
 * Use with Google Mock's EXPECT_CALL to set up expectations and return values.
 *
 * Example usage:
 * @code
 * auto mock_http = std::make_shared<MockHttpClient>();
 * EXPECT_CALL(*mock_http, post("/clusters/create", testing::_))
 *     .WillOnce(testing::Return(success_response()));
 * @endcode
 */
class MockHttpClient : public internal::IHttpClient {
public:
    virtual ~MockHttpClient() = default;

    MOCK_METHOD(internal::HttpResponse, get, (const std::string& path), (override));
    MOCK_METHOD(internal::HttpResponse, post, (const std::string& path, const std::string& json_body), (override));
    MOCK_METHOD(void, check_response, (const internal::HttpResponse& response, const std::string& operation_name),
                (const, override));

    // Helper methods to create common responses

    /**
     * @brief Create a successful HTTP 200 response
     */
    static internal::HttpResponse success_response(const std::string& body = "{}") {
        internal::HttpResponse response;
        response.status_code = 200;
        response.body = body;
        return response;
    }

    /**
     * @brief Create an HTTP 400 Bad Request response
     */
    static internal::HttpResponse bad_request_response(const std::string& error_message = "Bad Request") {
        internal::HttpResponse response;
        response.status_code = 400;
        response.body = R"({"error_code": "BAD_REQUEST", "message": ")" + error_message + R"("})";
        return response;
    }

    /**
     * @brief Create an HTTP 401 Unauthorized response
     */
    static internal::HttpResponse unauthorized_response() {
        internal::HttpResponse response;
        response.status_code = 401;
        response.body = R"({"error_code": "UNAUTHORIZED", "message": "Invalid authentication credentials"})";
        return response;
    }

    /**
     * @brief Create an HTTP 404 Not Found response
     */
    static internal::HttpResponse not_found_response(const std::string& resource = "Resource") {
        internal::HttpResponse response;
        response.status_code = 404;
        response.body = R"({"error_code": "NOT_FOUND", "message": ")" + resource + R"( not found"})";
        return response;
    }

    /**
     * @brief Create an HTTP 500 Internal Server Error response
     */
    static internal::HttpResponse server_error_response() {
        internal::HttpResponse response;
        response.status_code = 500;
        response.body = R"({"error_code": "INTERNAL_ERROR", "message": "Internal server error"})";
        return response;
    }

    /**
     * @brief Create a cluster creation success response
     */
    static internal::HttpResponse cluster_created_response(const std::string& cluster_id = "1234-567890-abcde123") {
        internal::HttpResponse response;
        response.status_code = 200;
        response.body = R"({"cluster_id": ")" + cluster_id + R"("})";
        return response;
    }
};

} // namespace test
} // namespace databricks