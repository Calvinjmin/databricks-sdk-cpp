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

# ========== Secrets API Examples ==========
add_executable(secrets_example secrets_example.cpp)
target_link_libraries(secrets_example PRIVATE databricks_sdk)

# Windows-specific definitions for all examples
if(MSVC)
    set(ALL_EXAMPLES simple_query jobs_example compute_example unity_catalog_example secrets_example)
    foreach(example ${ALL_EXAMPLES})
        target_compile_definitions(${example}
            PRIVATE
                NOMINMAX            # Prevent min/max macros
                NOGDI               # Prevent GDI macros (including ERROR)
                WIN32_LEAN_AND_MEAN # Reduce Windows header bloat
        )
    endforeach()
endif()

# Set RPATH for all examples to find ODBC libraries
set_target_properties(
    simple_query
    jobs_example
    compute_example
    unity_catalog_example
    secrets_example
    PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR};/opt/homebrew/lib;/usr/local/lib"
    INSTALL_RPATH "/opt/homebrew/lib;/usr/local/lib"
)
