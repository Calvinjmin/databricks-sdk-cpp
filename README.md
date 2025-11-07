# Databricks C++ SDK

[![Latest Release](https://img.shields.io/github/v/release/calvinjmin/databricks-sdk-cpp?display_name=tag&sort=semver)](https://github.com/calvinjmin/databricks-sdk-cpp/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Documentation](https://img.shields.io/badge/docs-online-blue.svg)](https://calvinjmin.github.io/databricks-sdk-cpp/)

A modern C++ SDK for Databricks with SQL, Jobs, and Compute APIs.

**Latest Release**: [v0.3.0](https://github.com/calvinjmin/databricks-sdk-cpp/releases/tag/v0.3.0) | **Documentation**: [calvinjmin.github.io/databricks-sdk-cpp](https://calvinjmin.github.io/databricks-sdk-cpp/)

---

## Table of Contents

- [Requirements](#requirements)
  - [ODBC Driver Setup](#odbc-driver-setup)
- [Installation](#installation)
  - [CMake FetchContent](#cmake-fetchcontent-recommended)
  - [vcpkg](#vcpkg)
  - [Manual Build](#manual-build)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Features](#features)
  - [Connection Pooling](#connection-pooling)
  - [Automatic Retries](#automatic-retries)
  - [Async Operations](#async-operations)
  - [Jobs API](#jobs-api)
  - [Compute API](#compute-api)
- [Building from Source](#building-from-source)
- [Running Examples](#running-examples)
- [Documentation](#documentation)
- [License](#license)

---

## Requirements

- **C++17 compiler**: GCC 7+, Clang 5+, MSVC 2017+
- **CMake**: 3.14 or higher
- **ODBC Driver Manager**:
  - Linux/macOS: unixODBC
  - Windows: Built-in ODBC Driver Manager
- **Simba Spark ODBC Driver**: [Download from Databricks](https://www.databricks.com/spark/odbc-drivers-download)

### ODBC Driver Setup

#### macOS

```bash
# Install unixODBC
brew install unixodbc

# Download and install Simba Spark ODBC Driver from Databricks
# Verify installation
odbcinst -q -d
```

You should see "Simba Spark ODBC Driver" in the output.

#### Linux

```bash
# Ubuntu/Debian
sudo apt-get install unixodbc unixodbc-dev

# RHEL/CentOS
sudo yum install unixODBC unixODBC-devel

# Download and install Simba Spark ODBC Driver from Databricks
# Verify installation
odbcinst -q -d
```

#### Windows

1. Download the Simba Spark ODBC Driver installer from [Databricks Downloads](https://www.databricks.com/spark/odbc-drivers-download)
2. Run the installer - it will automatically register with Windows ODBC Driver Manager

#### Verify Setup

Run the automated setup checker:

```bash
./scripts/check_odbc_setup.sh
```

This verifies unixODBC installation, configuration files, installed drivers, and library paths.

#### Alternative ODBC Drivers

To use a different ODBC driver:

```cpp
databricks::SQLConfig sql;
sql.odbc_driver_name = "Your Driver Name";  // Must match output from: odbcinst -q -d
```

---

## Installation

### CMake FetchContent (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  databricks_sdk
  GIT_REPOSITORY https://github.com/calvinjmin/databricks-sdk-cpp.git
  GIT_TAG main  # or specific version like v0.2.4
)

FetchContent_MakeAvailable(databricks_sdk)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

**Benefits**: No separate installation step, version-locked dependencies.

### vcpkg

Once published (submission in progress):

```bash
vcpkg install databricks-sdk-cpp
```

Then in your `CMakeLists.txt`:

```cmake
find_package(databricks_sdk CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

### Manual Build

```bash
git clone https://github.com/calvinjmin/databricks-sdk-cpp.git
cd databricks-sdk-cpp
mkdir build && cd build
cmake ..
cmake --build .

# Optional: Install system-wide
sudo cmake --install .
```

Then use in your project:

```cmake
find_package(databricks_sdk REQUIRED)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

---

## Quick Start

### Basic SQL Query

```cpp
#include <databricks/client.h>

int main() {
    // Automatically loads from ~/.databrickscfg or environment variables
    auto client = databricks::Client::Builder()
        .with_environment_config()
        .build();

    auto results = client.query("SELECT * FROM my_table LIMIT 10");

    for (const auto& row : results) {
        // Process results...
    }

    return 0;
}
```

### Manual Configuration

```cpp
#include <databricks/client.h>

int main() {
    // Configure authentication
    databricks::AuthConfig auth;
    auth.host = "https://my-workspace.databricks.com";
    auth.set_token("dapi1234567890abcdef");
    auth.timeout_seconds = 60;

    // Configure SQL settings
    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/abc123";

    // Build client
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .build();

    auto results = client.query("SELECT * FROM my_table");
    return 0;
}
```

---

## Configuration

The SDK supports multiple configuration methods with automatic precedence:

### Profile File (`~/.databrickscfg`)

Create `~/.databrickscfg`:

```ini
[DEFAULT]
host = https://my-workspace.databricks.com
token = dapi1234567890abcdef
http_path = /sql/1.0/warehouses/abc123

[production]
host = https://prod.databricks.com
token = dapi_prod_token
http_path = /sql/1.0/warehouses/prod123
```

Load specific profile:

```cpp
auto client = databricks::Client::Builder()
    .with_environment_config("production")
    .build();
```

### Environment Variables

```bash
export DATABRICKS_HOST="https://my-workspace.databricks.com"
export DATABRICKS_TOKEN="dapi1234567890abcdef"
export DATABRICKS_HTTP_PATH="/sql/1.0/warehouses/abc123"
export DATABRICKS_TIMEOUT=120  # Optional
```

Alternative variable names also supported:
- `DATABRICKS_SERVER_HOSTNAME`
- `DATABRICKS_ACCESS_TOKEN`
- `DATABRICKS_SQL_HTTP_PATH`

### Configuration Precedence

1. **Profile file** (`~/.databrickscfg`) - if present and complete, used exclusively
2. **Environment variables** - used as fallback

### Mixing Configuration

The Builder pattern allows flexible configuration:

```cpp
// Load auth from environment, customize pooling
databricks::PoolingConfig pooling;
pooling.enabled = true;
pooling.max_connections = 20;

auto client = databricks::Client::Builder()
    .with_environment_config()  // Load auth + SQL from environment
    .with_pooling(pooling)       // Override pooling
    .build();
```

---

## Features

### Connection Pooling

Eliminate connection overhead by reusing ODBC connections:

```cpp
databricks::PoolingConfig pooling;
pooling.enabled = true;
pooling.min_connections = 2;
pooling.max_connections = 10;

auto client = databricks::Client::Builder()
    .with_environment_config()
    .with_pooling(pooling)
    .build();

// Connections automatically acquired and released
auto results = client.query("SELECT * FROM my_table");
```

**Performance Impact:**
- Without pooling: 500-2000ms per query (includes connection setup)
- With pooling: 1-50ms per query (connection reused)

**Note:** Multiple clients with identical configuration automatically share the same pool.

### Automatic Retries

Built-in retry logic with exponential backoff for transient failures:

```cpp
databricks::RetryConfig retry;
retry.enabled = true;                      // Default: true
retry.max_attempts = 5;                    // Default: 3
retry.initial_backoff_ms = 200;            // Default: 100ms
retry.backoff_multiplier = 2.0;            // Default: 2.0
retry.max_backoff_ms = 10000;              // Default: 10000ms
retry.retry_on_timeout = true;             // Default: true
retry.retry_on_connection_lost = true;     // Default: true

auto client = databricks::Client::Builder()
    .with_environment_config()
    .with_retry(retry)
    .build();
```

**Retry Behavior:**
- **Retryable errors**: Connection timeouts, network errors, 429/503/502/504 responses
- **Non-retryable errors**: Authentication failures, SQL syntax errors, permission errors
- **Exponential backoff**: With jitter to prevent thundering herd
- **Enabled by default** with sensible settings

### Async Operations

Non-blocking connection and query execution:

```cpp
auto client = databricks::Client::Builder()
    .with_environment_config()
    .with_auto_connect(false)  // Disable automatic connection
    .build();

// Start connecting asynchronously
auto connect_future = client.connect_async();

// Do other initialization work...

// Wait for connection before querying
connect_future.wait();
auto results = client.query("SELECT current_timestamp()");
```

**Use Cases:**
- Reduce perceived latency during application startup
- Execute multiple queries concurrently
- Combine with pooling for maximum throughput

### Jobs API

Automate and orchestrate Databricks workflows:

```cpp
#include <databricks/jobs.h>

databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
databricks::Jobs jobs(auth);

// List all jobs
auto job_list = jobs.list_jobs(25, 0);  // limit, offset
for (const auto& job : job_list) {
    std::cout << "Job: " << job.name << " (ID: " << job.job_id << ")" << std::endl;
}

// Get job details
auto job = jobs.get_job(123456);
std::cout << "Created by: " << job.creator_user_name << std::endl;

// Trigger a job run with parameters
std::map<std::string, std::string> params;
params["date"] = "2024-01-01";
params["environment"] = "production";

uint64_t run_id = jobs.run_now(123456, params);
std::cout << "Started run: " << run_id << std::endl;
```

**Features:**
- Paginated job listing
- Full job metadata retrieval
- Job execution with parameters
- Type-safe IDs (uint64_t)

### Compute API

Manage Databricks compute clusters programmatically:

```cpp
#include <databricks/compute/compute.h>

databricks::AuthConfig auth = databricks::AuthConfig::from_environment();
databricks::Compute compute(auth);

// List all clusters
auto clusters = compute.list_compute();
for (const auto& cluster : clusters) {
    std::cout << cluster.cluster_name << " [" << cluster.state << "]" << std::endl;
}

// Get cluster details
auto cluster = compute.get_compute("cluster-id");

// Manage cluster lifecycle
compute.start_compute("cluster-id");
compute.restart_compute("cluster-id");
compute.terminate_compute("cluster-id");
```

**Features:**
- List and get cluster details
- Start, restart, and terminate clusters
- Cluster state tracking (PENDING, RUNNING, TERMINATED, etc.)
- Automatic HTTP retry with exponential backoff

---

## Building from Source

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests (optional)
cmake --build . --target unit_tests
./tests/unit_tests

# Install (optional)
sudo cmake --install .
```

### Build Options

```bash
cmake -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON -DBUILD_SHARED_LIBS=ON ..
```

Available options:
- `BUILD_EXAMPLES` (default: ON) - Build example applications
- `BUILD_TESTS` (default: OFF) - Build unit tests
- `BUILD_SHARED_LIBS` (default: ON) - Build as shared library

---

## Running Examples

After building with `BUILD_EXAMPLES=ON`:

```bash
# SQL query execution
./build/examples/simple_query

# Jobs API - list, get details, trigger runs
./build/examples/jobs_example

# Compute API - manage clusters
./build/examples/compute_example
```

**Configuration Required:** Examples use `with_environment_config()`, so you must configure either:
- Profile file: `~/.databrickscfg` with `[DEFAULT]` section
- Environment variables: `DATABRICKS_HOST`, `DATABRICKS_TOKEN`, `DATABRICKS_HTTP_PATH`

---

## Documentation

### Online Documentation

**Live Docs**: [calvinjmin.github.io/databricks-sdk-cpp](https://calvinjmin.github.io/databricks-sdk-cpp/)

Automatically generated and published via GitHub Actions on every commit to `main`.

### Generate Documentation Locally

```bash
# Install Doxygen
brew install doxygen  # macOS
# or: sudo apt-get install doxygen  # Linux

# Generate documentation
doxygen Doxyfile

# View in browser
open docs/html/index.html  # macOS
# or: xdg-open docs/html/index.html  # Linux
```

### Documentation Contents

- Complete API reference for all classes and methods
- Code examples from header comments
- SQL Client, Jobs API, Compute API documentation
- Connection pooling and configuration reference
- Source code browser with syntax highlighting
- Search functionality

---

## Contributing

Contributions are welcome! We appreciate bug reports, feature requests, documentation improvements, and code contributions.

### How to Contribute

1. **Report Issues**: Found a bug? [Open an issue](https://github.com/calvinjmin/databricks-sdk-cpp/issues/new/choose)
2. **Suggest Features**: Have an idea? [Start a discussion](https://github.com/calvinjmin/databricks-sdk-cpp/discussions)
3. **Submit Changes**: See our [Contributing Guide](CONTRIBUTING.md) for details

### Quick Start for Contributors

```bash
# Fork and clone
git clone https://github.com/YOUR_USERNAME/databricks-sdk-cpp.git
cd databricks-sdk-cpp

# Build and test
make build-all
make test

# Make changes, then format
make format

# Submit a pull request
```

### Good First Issues

New contributors can look for issues labeled [`good first issue`](https://github.com/calvinjmin/databricks-sdk-cpp/labels/good%20first%20issue) to get started.

## License

MIT License - see [LICENSE](./LICENSE) file for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/calvinjmin/databricks-sdk-cpp/issues)
- **Discussions**: [GitHub Discussions](https://github.com/calvinjmin/databricks-sdk-cpp/discussions)
- **Security**: See [SECURITY.md](SECURITY.md) for vulnerability reporting

**Author**: Calvin Min (calvinjmin@gmail.com)
