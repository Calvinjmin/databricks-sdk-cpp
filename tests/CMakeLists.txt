# Test configuration organized by test type
include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# ========== Unit Tests (Google Test with GMock) ==========
file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "unit/**/*.cpp")
add_executable(unit_tests ${UNIT_TEST_SOURCES})

# Find fmt package (spdlog dependency)
find_package(fmt QUIET)

target_link_libraries(unit_tests
    PRIVATE
        databricks_sdk
        GTest::gtest_main
        GTest::gmock_main
)

# Link fmt if found (needed on some systems where spdlog uses external fmt)
if(fmt_FOUND)
    target_link_libraries(unit_tests PRIVATE fmt::fmt)
endif()

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Discover and register Google Test cases
include(GoogleTest)
gtest_discover_tests(unit_tests)