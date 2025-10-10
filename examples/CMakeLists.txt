# Example applications organized by feature

# ========== Basic Examples ==========
add_executable(simple_query basic/simple_query.cpp)
target_link_libraries(simple_query PRIVATE databricks_sdk)

add_executable(secure_query basic/secure_query.cpp)
target_link_libraries(secure_query PRIVATE databricks_sdk)

add_executable(modular_config_example basic/modular_config_example.cpp)
target_link_libraries(modular_config_example PRIVATE databricks_sdk)

# ========== Connection Pooling Examples ==========
add_executable(transparent_pooling pooling/transparent_pooling.cpp)
target_link_libraries(transparent_pooling PRIVATE databricks_sdk)

add_executable(benchmark pooling/benchmark.cpp)
target_link_libraries(benchmark PRIVATE databricks_sdk)

# ========== Async Examples ==========
add_executable(async_operations async/async_operations.cpp)
target_link_libraries(async_operations PRIVATE databricks_sdk)

add_executable(pool_async_combined async/pool_async_combined.cpp)
target_link_libraries(pool_async_combined PRIVATE databricks_sdk)

# Set RPATH for all examples to find ODBC libraries
set_target_properties(
    simple_query
    secure_query
    modular_config_example
    transparent_pooling
    benchmark
    async_operations
    pool_async_combined
    PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR};/opt/homebrew/lib;/usr/local/lib"
    INSTALL_RPATH "/opt/homebrew/lib;/usr/local/lib"
)
