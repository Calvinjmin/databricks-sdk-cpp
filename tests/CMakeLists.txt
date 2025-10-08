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

# ========== Unit Tests (Google Test) ==========
add_executable(unit_tests
    unit/test_client.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        databricks_sdk
        GTest::gtest_main
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

# Discover and register Google Test cases
include(GoogleTest)
gtest_discover_tests(unit_tests)