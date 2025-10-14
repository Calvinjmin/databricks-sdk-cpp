#pragma once

#include "databricks/config.h"
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
         * @brief Internal HTTP Client Wrapper around libcurl
         */
        class HttpClient {
            public:
                explicit HttpClient( const AuthConfig& auth );

                /**
                 * @brief Wrapper around a GET REST API Call
                 * 
                 * @param path URL Path to Get Request
                 * 
                 * @return HttpResponse HTTP Response Object
                 */
                HttpResponse get( const std::string &path );

                /**
                 * @brief Wrapper around a POST REST API Call
                 * 
                 * @param path URL Path to Post Request
                 * @param json_body Request Payload
                 * 
                 * @return HttpResponse HTTP Response Object
                 */
                HttpResponse post( const std::string &path, const std::string &json_body );

            private:
                AuthConfig auth_;
                std::string get_base_url() const;
                std::map<std::string, std::string> get_headers() const;
        };
    } // namespace internal
} // namespace databricks