# Example applications demonstrating SDK features

# ========== SQL Examples ==========
add_executable(simple_query simple_query.cpp)
target_link_libraries(simple_query PRIVATE databricks_sdk)

# ========== Jobs API Examples ==========
add_executable(jobs_example jobs_example.cpp)
target_link_libraries(jobs_example PRIVATE databricks_sdk)

# ========== Compute API Examples ==========
add_executable(compute_example compute_example.cpp)
target_link_libraries(compute_example PRIVATE databricks_sdk)

# ========== Unity Catalog API Examples ==========
add_executable(unity_catalog_example unity_catalog_example.cpp)
target_link_libraries(unity_catalog_example PRIVATE databricks_sdk)

# Set RPATH for all examples to find ODBC libraries
set_target_properties(
    simple_query
    jobs_example
    compute_example
    unity_catalog_example
    PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR};/opt/homebrew/lib;/usr/local/lib"
    INSTALL_RPATH "/opt/homebrew/lib;/usr/local/lib"
)
