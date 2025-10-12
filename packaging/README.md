# Packaging

This directory contains files for distributing the Databricks C++ SDK through various package managers.

## Directory Structure

```
packaging/
├── vcpkg-port/          # vcpkg registry port files
│   ├── portfile.cmake   # Build and installation instructions for vcpkg
│   ├── vcpkg.json       # Port metadata (dependencies, version, etc.)
│   └── usage            # User-facing installation guide
└── README.md            # This file
```

## vcpkg Port

The `vcpkg-port/` directory contains files needed to publish the SDK to the [vcpkg registry](https://github.com/microsoft/vcpkg).

### Files

- **`portfile.cmake`**: Tells vcpkg how to download, build, and install the SDK
- **`vcpkg.json`**: Declares the port metadata (name, version, dependencies, platform support)
- **`usage`**: Instructions shown to users after they install the package

### How to Use

When submitting to vcpkg or updating the port, copy these files to the vcpkg repository:

```bash
# In your vcpkg clone
mkdir -p ports/databricks-sdk-cpp
cp packaging/vcpkg-port/* ports/databricks-sdk-cpp/
```

For complete submission instructions, see [dev-docs/VCPKG_SUBMISSION.md](../dev-docs/VCPKG_SUBMISSION.md).

## Future Package Managers

This directory will eventually include packaging files for:
- **Conan** (`conan/` directory) - Cross-platform C++ package manager
- **Homebrew** (`homebrew/` directory) - macOS package manager
- **apt/yum** - Linux distribution packages

## For End Users

End users don't need to interact with this directory directly. To install the SDK:
- **vcpkg**: `vcpkg install databricks-sdk-cpp`
- **FetchContent**: See [README.md](../README.md#option-2-cmake-fetchcontent-direct-from-github)
- **Manual**: See [README.md](../README.md#option-3-manual-build-and-install)

