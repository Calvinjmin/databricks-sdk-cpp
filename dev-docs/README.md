# Developer Documentation

This directory contains documentation for maintainers and contributors of the Databricks C++ SDK.

## Contents

### 📦 [VCPKG_SUBMISSION.md](VCPKG_SUBMISSION.md)
Complete guide for publishing the SDK to the vcpkg package registry. Includes:
- Step-by-step submission process
- Testing procedures
- Versioning and updates
- Troubleshooting common issues

**Audience**: Package maintainers publishing to vcpkg

### 📦 [Packaging Files](../packaging/)
Distribution and packaging files have been moved to the `packaging/` directory at the project root. This includes:
- `packaging/vcpkg-port/` - vcpkg registry port files
- Future: Conan, Homebrew, and other package manager configs

**See**: [packaging/README.md](../packaging/README.md) for details

## For End Users

If you're looking to **use** the SDK (not develop/maintain it), see the main [README.md](../README.md) for:
- Installation options (vcpkg, FetchContent, manual build)
- Quick start guide
- Configuration examples
- API usage

## Contributing

If you'd like to contribute to the SDK:
1. Check out the main [README.md](../README.md) for build instructions
2. See [tests/README.md](../tests/README.md) for information on running tests
3. Review the existing codebase structure

## Other Documentation

- **API Reference**: Generate with `make docs` (requires Doxygen)
- **Examples**: See [examples/](../examples/) directory for working code samples
- **Tests**: See [tests/README.md](../tests/README.md) for test documentation

