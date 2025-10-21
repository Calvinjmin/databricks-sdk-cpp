cmake_minimum_required(VERSION 3.14)
project(databricks_sdk
    VERSION 0.2.2
    DESCRIPTION "Databricks C++ SDK"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Platform-specific configuration
if(WIN32)
    # Windows uses odbc32
    set(ODBC_LIBRARIES odbc32)
elseif(APPLE)
    # macOS: Prefer Homebrew paths on Apple Silicon
    if(NOT CMAKE_PREFIX_PATH AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    endif()

    # Find ODBC using macOS paths
    find_path(ODBC_INCLUDE_DIR sql.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /opt/local/include
        PATH_SUFFIXES odbc
        NO_DEFAULT_PATH)

    find_library(ODBC_LIBRARY
        NAMES odbc iodbc unixodbc
        PATHS
            /opt/homebrew/lib
            /usr/local/lib
            /opt/local/lib
        NO_DEFAULT_PATH)

    if(ODBC_LIBRARY)
        set(ODBC_LIBRARIES ${ODBC_LIBRARY})
        message(STATUS "Found ODBC library: ${ODBC_LIBRARY}")
        if(ODBC_INCLUDE_DIR)
            set(ODBC_INCLUDE_DIRS ${ODBC_INCLUDE_DIR})
            message(STATUS "Found ODBC include: ${ODBC_INCLUDE_DIR}")
        endif()
    else()
        message(FATAL_ERROR
            "ODBC library not found. Please install:\n"
            "  brew install unixodbc")
    endif()
else()
    # Linux: Use standard paths
    find_path(ODBC_INCLUDE_DIR sql.h
        PATHS
            /usr/include
            /usr/local/include
        PATH_SUFFIXES odbc
        NO_DEFAULT_PATH)

    find_library(ODBC_LIBRARY
        NAMES odbc unixodbc
        PATHS
            /usr/lib
            /usr/local/lib
        NO_DEFAULT_PATH)

    if(ODBC_LIBRARY)
        set(ODBC_LIBRARIES ${ODBC_LIBRARY})
        message(STATUS "Found ODBC library: ${ODBC_LIBRARY}")
        if(ODBC_INCLUDE_DIR)
            set(ODBC_INCLUDE_DIRS ${ODBC_INCLUDE_DIR})
            message(STATUS "Found ODBC include: ${ODBC_INCLUDE_DIR}")
        endif()
    else()
        message(FATAL_ERROR
            "ODBC library not found. Please install:\n"
            "  Debian/Ubuntu: sudo apt-get install unixodbc-dev\n"
            "  RedHat/CentOS: sudo yum install unixODBC-devel")
    endif()
endif()

# External libraries
find_package(spdlog CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Library sources
set(SOURCES
    src/core/client.cpp
    src/core/config.cpp
    src/jobs/jobs.cpp
    src/compute/compute_types.cpp
    src/compute/compute.cpp
    src/connection_pool.cpp
    src/unity_catalog/unity_catalog.cpp
    src/internal/pool_manager.cpp
    src/internal/logger.cpp
    src/internal/http_client.cpp
)

set(HEADERS
    include/databricks/core/client.h
    include/databricks/core/config.h
    include/databricks/connection_pool.h
    include/databricks/version.h
    include/databricks/jobs/jobs.h
    include/databricks/compute/compute.h
    include/databricks/compute/compute_types.h
    include/databricks/unity_catalog/unity_catalog.h
    include/databricks/unity_catalog/unity_catalog_types.h
)

# Internal headers (not installed)
set(INTERNAL_HEADERS
    src/internal/pool_manager.h
    src/internal/logger.h
    src/internal/http_client.h
)

# Create library target
add_library(databricks_sdk ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(databricks_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${ODBC_INCLUDE_DIRS}
)

# Link ODBC libraries
target_link_libraries(databricks_sdk PRIVATE
    ${ODBC_LIBRARIES}
    spdlog::spdlog
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Set library properties
set_target_properties(databricks_sdk PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Set RPATH for finding ODBC libraries at runtime (Unix/Linux/macOS only)
if(NOT WIN32)
    # Use RPATH to find shared libraries relative to executable
    set_target_properties(databricks_sdk PROPERTIES
        BUILD_RPATH "${ODBC_LIBRARY_DIR};/usr/local/lib;/opt/homebrew/lib;/opt/local/lib"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;/usr/local/lib;/opt/homebrew/lib;/opt/local/lib"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(databricks_sdk PRIVATE /W4)
else()
    target_compile_options(databricks_sdk PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS databricks_sdk
    EXPORT databricks_sdk-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers with directory structure preserved
install(DIRECTORY include/databricks/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/databricks
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT databricks_sdk-targets
    FILE databricks_sdk-targets.cmake
    NAMESPACE databricks_sdk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/databricks_sdk
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/databricks_sdk-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/databricks_sdk-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/databricks_sdk-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/databricks_sdk
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/databricks_sdk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/databricks_sdk-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/databricks_sdk
)
