# Port for databricks-sdk-cpp
# This file will be submitted to the vcpkg registry at:
# https://github.com/microsoft/vcpkg/tree/master/ports/databricks-sdk-cpp

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO calvinjmin/databricks-sdk-cpp
    REF "v${VERSION}"
    SHA512 0  # This will be updated during submission with actual hash
    HEAD_REF main
)

# Check for ODBC availability
vcpkg_find_acquire_msys(MSYS_ROOT)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_EXAMPLES=OFF
        -DBUILD_TESTS=OFF
)

vcpkg_cmake_install()

# Move cmake config files to the right location
vcpkg_cmake_config_fixup(
    PACKAGE_NAME databricks_sdk
    CONFIG_PATH lib/cmake/databricks_sdk
)

# Remove duplicate files from debug
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Install license
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Copy usage file
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

