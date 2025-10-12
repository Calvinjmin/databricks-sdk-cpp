# vcpkg Submission Guide

This document provides instructions for publishing `databricks-sdk-cpp` to vcpkg.

## What is vcpkg?

vcpkg is a free C/C++ package manager for acquiring and managing libraries. It's maintained by Microsoft and provides:
- Cross-platform support (Windows, Linux, macOS)
- Easy CMake integration
- Binary caching for faster builds
- Large, active community

## Quick Start for Users

Once published to vcpkg, users can install your library with:

```bash
# Install vcpkg if not already installed
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows

# Install databricks-sdk-cpp
./vcpkg install databricks-sdk-cpp
```

Then in their CMake project:
```cmake
find_package(databricks_sdk CONFIG REQUIRED)
target_link_libraries(main PRIVATE databricks_sdk::databricks_sdk)
```

## Submission Process

### 1. Prepare Your Release

**Before submitting to vcpkg, you need a tagged release:**

```bash
# Tag the current version
git tag -a v0.1.0 -m "Release v0.1.0 - Initial release"
git push origin v0.1.0
```

### 2. Set Up vcpkg Repository

Fork and clone the vcpkg repository:

```bash
# Fork https://github.com/microsoft/vcpkg on GitHub first

# Clone your fork
git clone https://github.com/YOUR-USERNAME/vcpkg.git
cd vcpkg

# Add upstream
git remote add upstream https://github.com/microsoft/vcpkg.git

# Create a branch
git checkout -b add-databricks-sdk-cpp
```

### 3. Create the Port

Copy the template files:

```bash
# Create port directory
mkdir -p ports/databricks-sdk-cpp

# Copy files from the template directory in this repo
cp /path/to/databricks-sdk-cpp/vcpkg-port-template/* ports/databricks-sdk-cpp/
```

### 4. Generate SHA512 Hash

Download your release and generate its SHA512:

```bash
# Download the release tarball
curl -L -o source.tar.gz https://github.com/calvinjmin/databricks-sdk-cpp/archive/refs/tags/v0.1.0.tar.gz

# Generate SHA512 hash
shasum -a 512 source.tar.gz
# or: sha512sum source.tar.gz (Linux)
# or: Get-FileHash -Algorithm SHA512 source.tar.gz (PowerShell)
```

**Update `portfile.cmake`** with the hash:
```cmake
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO calvinjmin/databricks-sdk-cpp
    REF "v${VERSION}"
    SHA512 YOUR_GENERATED_HASH_HERE  # ← Update this line
    HEAD_REF main
)
```

### 5. Test Locally

Test the port on your system:

```bash
# From vcpkg root directory
./vcpkg remove databricks-sdk-cpp  # Remove if already installed
./vcpkg install databricks-sdk-cpp --overlay-ports=./ports

# Test the integration
./vcpkg install databricks-sdk-cpp --overlay-ports=./ports --editable
```

**Test on multiple platforms if possible:**
- Windows: `./vcpkg install databricks-sdk-cpp:x64-windows`
- Linux: `./vcpkg install databricks-sdk-cpp:x64-linux`
- macOS: `./vcpkg install databricks-sdk-cpp:x64-osx`

### 6. Update vcpkg Baseline

```bash
# From vcpkg root
git add ports/databricks-sdk-cpp

# Run vcpkg format check
./vcpkg format-manifest ports/databricks-sdk-cpp/vcpkg.json

# Update versions
./vcpkg x-add-version databricks-sdk-cpp

# This will update versions/baseline.json and versions/d-/databricks-sdk-cpp.json
```

### 7. Commit and Push

```bash
git add ports/databricks-sdk-cpp versions/
git commit -m "[databricks-sdk-cpp] new port

- Add databricks-sdk-cpp version 0.1.0
- C++ SDK for Databricks with ODBC support
"

git push origin add-databricks-sdk-cpp
```

### 8. Create Pull Request

1. Go to https://github.com/microsoft/vcpkg/pulls
2. Click "New Pull Request"
3. Select your branch
4. Title: `[databricks-sdk-cpp] new port`
5. Fill out the template:

```markdown
## Description
This PR adds databricks-sdk-cpp, a C++ SDK for Databricks that provides ODBC-based connectivity.

- Repository: https://github.com/calvinjmin/databricks-sdk-cpp
- Version: 0.1.0
- License: MIT

## Checklist
- [x] I have read the [contributing guidelines](https://github.com/microsoft/vcpkg/blob/master/docs/contributing.md)
- [x] The port builds successfully on my system
- [x] vcpkg x-add-version has been run
- [x] All tests pass locally

## Dependencies
- Requires ODBC driver manager (system dependency)
- Requires Simba Spark ODBC Driver (user must install separately)

## Platform Support
- Windows: Yes (tested)
- Linux: Yes (tested)  
- macOS: Yes (tested)
```

### 9. Respond to CI and Reviews

- vcpkg CI will test your port on Windows, Linux, and macOS
- Address any failures or reviewer comments
- Common issues:
  - Missing system dependencies
  - Incorrect SHA512
  - License file not installed correctly
  - CMake config issues

## Common Issues and Solutions

### Issue: SHA512 Mismatch
**Solution**: Regenerate the hash and ensure you're using the exact release tag.

### Issue: Build Fails Due to ODBC
**Solution**: ODBC is a system dependency. Document it clearly in the usage file.

### Issue: CMake Config Not Found
**Solution**: Ensure `vcpkg_cmake_config_fixup` has the correct `CONFIG_PATH`.

### Issue: Port Only Works on One Platform
**Solution**: Add platform-specific conditionals in portfile.cmake or limit support in vcpkg.json:
```json
"supports": "!(uwp | arm)"
```

## Alternative Distribution (While Waiting for Approval)

You can share your library via vcpkg before it's in the main registry:

### Option 1: Custom Registry (Recommended)

Create `vcpkg-configuration.json` in your repo:
```json
{
  "default-registry": {
    "kind": "git",
    "repository": "https://github.com/microsoft/vcpkg"
  },
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/calvinjmin/databricks-sdk-cpp-vcpkg-registry",
      "baseline": "commit-hash",
      "packages": ["databricks-sdk-cpp"]
    }
  ]
}
```

### Option 2: Direct GitHub Integration

Users can install directly from source using FetchContent (already supported!):

```cmake
include(FetchContent)
FetchContent_Declare(
  databricks_sdk
  GIT_REPOSITORY https://github.com/calvinjmin/databricks-sdk-cpp.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(databricks_sdk)
target_link_libraries(main PRIVATE databricks_sdk::databricks_sdk)
```

## Keeping the Port Updated

When releasing new versions:

1. Update version in CMakeLists.txt
2. Create and push new tag
3. Generate new SHA512
4. Submit update PR to vcpkg: `[databricks-sdk-cpp] update to X.Y.Z`

## Resources

- [vcpkg Documentation](https://vcpkg.io/)
- [Contributing to vcpkg](https://github.com/microsoft/vcpkg/blob/master/docs/contributing.md)
- [Portfile Functions Reference](https://learn.microsoft.com/en-us/vcpkg/maintainers/functions/)
- [Example Ports](https://github.com/microsoft/vcpkg/tree/master/ports)

## Need Help?

- vcpkg Discussions: https://github.com/microsoft/vcpkg/discussions
- vcpkg Slack: https://cppalliance.org/slack/

