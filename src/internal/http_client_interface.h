#pragma once

#include "databricks/core/config.h"
#include <string>
#include <map>

namespace databricks {
    namespace internal {
        /**
         * @brief Simple HTTP Response
         */
        struct HttpResponse {
            int status_code;
            std::string body;
            std::map<std::string, std::string> headers;
        };

        /**
         * @brief Interface for HTTP Client operations
         *
         * This interface allows dependency injection for testing.
         * Production code uses HttpClient, tests can inject MockHttpClient.
         */
        class IHttpClient {
        public:
            virtual ~IHttpClient() = default;

            /**
             * @brief Perform a GET request
             *
             * @param path URL path for the GET request
             * @return HttpResponse HTTP response object
             */
            virtual HttpResponse get(const std::string& path) = 0;

            /**
             * @brief Perform a POST request
             *
             * @param path URL path for the POST request
             * @param json_body Request payload as JSON string
             * @return HttpResponse HTTP response object
             */
            virtual HttpResponse post(const std::string& path, const std::string& json_body) = 0;

            /**
             * @brief Check HTTP response and throw on error
             *
             * @param response HTTP response to validate
             * @param operation_name Name of the operation for error messages
             * @throws std::runtime_error if response indicates an error
             */
            virtual void check_response(const HttpResponse& response, const std::string& operation_name) const = 0;
        };
    } // namespace internal
} // namespace databricks
