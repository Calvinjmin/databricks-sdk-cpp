#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace databricks {
    namespace internal {
        /**
         * @brief Get an instance of the spdLogger
         * 
         * Internal Logger object for outputs
         *   Log Level from DATABRICKS_LOG_LEVEL (default: INFO)
         */
        std::shared_ptr<spdlog::logger> get_logger();
    } // namespace internal
} // namespace databricks