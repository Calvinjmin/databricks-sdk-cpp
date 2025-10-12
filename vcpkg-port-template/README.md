# vcpkg Port Template

This directory contains the files needed to submit `databricks-sdk-cpp` to the vcpkg registry.

## Files

- **`portfile.cmake`**: Build and installation instructions for vcpkg
- **`vcpkg.json`**: Port metadata (name, version, dependencies)
- **`usage`**: Usage instructions shown to users after installation

## How to Submit to vcpkg

### Prerequisites

1. **Tag a release** in your GitHub repository:
   ```bash
   git tag -a v0.1.0 -m "Release version 0.1.0"
   git push origin v0.1.0
   ```

2. **Fork and clone vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   # Or fork first and clone your fork
   ```

### Step 1: Create the Port

1. Create the port directory:
   ```bash
   cd vcpkg/ports
   mkdir databricks-sdk-cpp
   ```

2. Copy files from this template directory:
   ```bash
   cp /path/to/databricks-sdk-cpp/vcpkg-port-template/* databricks-sdk-cpp/
   ```

### Step 2: Generate SHA512 Hash

After tagging your release, generate the SHA512 hash:

```bash
# Download the release tarball
curl -L -o source.tar.gz https://github.com/calvinjmin/databricks-sdk-cpp/archive/refs/tags/v0.1.0.tar.gz

# Generate SHA512
shasum -a 512 source.tar.gz

# Or on Linux:
sha512sum source.tar.gz
```

Update the `SHA512` field in `portfile.cmake` with this hash.

### Step 3: Test the Port Locally

```bash
# From vcpkg root
./vcpkg install databricks-sdk-cpp --overlay-ports=./ports/databricks-sdk-cpp

# Test on multiple platforms (if possible)
./vcpkg install databricks-sdk-cpp:x64-windows --overlay-ports=./ports/databricks-sdk-cpp
./vcpkg install databricks-sdk-cpp:x64-linux --overlay-ports=./ports/databricks-sdk-cpp
./vcpkg install databricks-sdk-cpp:x64-osx --overlay-ports=./ports/databricks-sdk-cpp
```

### Step 4: Create Pull Request

1. Create a branch in your vcpkg fork:
   ```bash
   git checkout -b add-databricks-sdk-cpp
   git add ports/databricks-sdk-cpp
   git commit -m "[databricks-sdk-cpp] new port"
   git push origin add-databricks-sdk-cpp
   ```

2. Open a PR to microsoft/vcpkg with the title:
   ```
   [databricks-sdk-cpp] new port
   ```

3. Fill out the PR template with:
   - Description of the library
   - Link to your GitHub repository
   - Confirmation that you tested on supported platforms

### Step 5: Respond to CI and Reviewers

- vcpkg's CI will test your port on multiple platforms
- Address any issues found by CI or reviewers
- Common issues:
  - Missing dependencies
  - Platform-specific build failures
  - License file installation
  - cmake config fixup

## Alternative: Using vcpkg Directly (Without Registry)

Users can use your library via vcpkg without waiting for registry approval:

### Option 1: Git Registry

Add to `vcpkg-configuration.json`:
```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/calvinjmin/databricks-sdk-cpp",
      "baseline": "commit-hash-here",
      "packages": ["databricks-sdk-cpp"]
    }
  ]
}
```

### Option 2: Overlay Ports

Users can copy the port files and use overlay:
```bash
vcpkg install databricks-sdk-cpp --overlay-ports=/path/to/port
```

## Versioning

When updating the library:

1. Update version in:
   - `CMakeLists.txt` (project VERSION)
   - Root `vcpkg.json`
   - Port template `vcpkg.json`

2. Tag the new release:
   ```bash
   git tag -a v0.2.0 -m "Release version 0.2.0"
   git push origin v0.2.0
   ```

3. Submit a new PR to vcpkg:
   ```
   [databricks-sdk-cpp] update to 0.2.0
   ```

## Resources

- [vcpkg Contributing Guide](https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/registries.md)
- [vcpkg Portfile Functions](https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/vcpkg_functions.md)
- [Example Ports](https://github.com/microsoft/vcpkg/tree/master/ports)

