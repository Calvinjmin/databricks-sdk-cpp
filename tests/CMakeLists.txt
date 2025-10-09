# Test configuration organized by test type
include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# ========== Unit Tests (Google Test with GMock) ==========
add_executable(unit_tests
    unit/test_client.cpp
    unit/test_config.cpp
    unit/test_connection_pool.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        databricks_sdk
        GTest::gtest_main
        GTest::gmock_main
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Discover and register Google Test cases
include(GoogleTest)
gtest_discover_tests(unit_tests)