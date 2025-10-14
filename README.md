# Databricks C++ SDK

[![Latest Release](https://img.shields.io/github/v/release/calvinjmin/databricks-sdk-cpp?display_name=tag&sort=semver)](https://github.com/calvinjmin/databricks-sdk-cpp/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Documentation](https://img.shields.io/badge/docs-online-blue.svg)](https://calvinjmin.github.io/databricks-sdk-cpp/)

A C++ SDK for Databricks, providing an interface for interacting with Databricks services.

**Latest Release**: [v0.1.0](https://github.com/calvinjmin/databricks-sdk-cpp/releases/tag/v0.1.0)

**Author**: Calvin Min (calvinjmin@gmail.com)

## Table of Contents

- [Requirements](#requirements)
  - [ODBC Driver Setup](#odbc-driver-setup)
  - [Automated Setup Check](#automated-setup-check)
- [Installation](#installation)
  - [Option 1: CMake FetchContent (Recommended)](#option-1-cmake-fetchcontent-recommended---direct-from-github)
  - [Option 2: vcpkg](#option-2-vcpkg)
  - [Option 3: Manual Build and Install](#option-3-manual-build-and-install)
- [Building from Source](#building-from-source)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Running Examples](#running-examples)
- [Performance Considerations](#performance-considerations)
- [Advanced Usage](#advanced-usage)
- [Documentation](#documentation)
- [License](#license)
- [Contributing](#contributing)

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 or higher
- **ODBC Driver Manager**:
  - **Linux/macOS**: unixODBC (`brew install unixodbc` or `apt-get install unixodbc-dev`)
  - **Windows**: Built-in ODBC Driver Manager
- **Simba Spark ODBC Driver**: [Download from Databricks](https://www.databricks.com/spark/odbc-drivers-download)

### ODBC Driver Setup

After installing the requirements above, you need to configure the ODBC driver:

#### Linux/macOS

1. **Install unixODBC** (if not already installed):
   ```bash
   # macOS
   brew install unixodbc

   # Ubuntu/Debian
   sudo apt-get install unixodbc unixodbc-dev

   # RedHat/CentOS
   sudo yum install unixODBC unixODBC-devel
   ```

2. **Download and install Simba Spark ODBC Driver** from [Databricks Downloads](https://www.databricks.com/spark/odbc-drivers-download)

3. **Verify driver installation**:
   ```bash
   odbcinst -q -d
   ```
   You should see "Simba Spark ODBC Driver" in the output.

4. **If driver is not found**, check ODBC configuration locations:
   ```bash
   odbcinst -j
   ```
   Ensure the driver is registered in the `odbcinst.ini` file shown.

#### Windows

1. Download and run the Simba Spark ODBC Driver installer from [Databricks Downloads](https://www.databricks.com/spark/odbc-drivers-download)
2. The installer will automatically register the driver with Windows ODBC Driver Manager

#### Using Alternative ODBC Drivers

If you prefer to use a different ODBC driver, you can configure it:

```cpp
databricks::SQLConfig sql;
sql.odbc_driver_name = "Your Driver Name Here"; // Must match driver name from odbcinst -q -d

auto client = databricks::Client::Builder()
    .with_environment_config()
    .with_sql(sql)
    .build();
```

### Automated Setup Check

Run the setup checker script to verify your ODBC configuration:

```bash
./scripts/check_odbc_setup.sh
```

This will verify:
- unixODBC installation
- ODBC configuration files
- Installed ODBC drivers (including Simba Spark)
- Library paths

## Installation

### Option 1: CMake FetchContent (Recommended - Direct from GitHub)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  databricks_sdk
  GIT_REPOSITORY https://github.com/calvinjmin/databricks-sdk-cpp.git
  GIT_TAG v0.1.0  # or use 'main' for latest
)

FetchContent_MakeAvailable(databricks_sdk)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

**Advantages**: No separate installation step, always gets the exact version you specify.

### Option 2: vcpkg

Once published to vcpkg (submission in progress), install with:

```bash
vcpkg install databricks-sdk-cpp
```

Then use in your CMake project:
```cmake
find_package(databricks_sdk CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

For maintainers: See [dev-docs/VCPKG_SUBMISSION.md](dev-docs/VCPKG_SUBMISSION.md) for the complete submission guide.

### Option 3: Manual Build and Install

```bash
# Clone and build
git clone https://github.com/calvinjmin/databricks-sdk-cpp.git
cd databricks-sdk-cpp
mkdir build && cd build
cmake ..
cmake --build .

# Install (requires sudo on Linux/macOS)
sudo cmake --install .
```

Then use in your project:

```cmake
find_package(databricks_sdk REQUIRED)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

## Building from Source

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
sudo cmake --install .
```

### Build Options

- `BUILD_EXAMPLES` (default: ON) - Build example applications
- `BUILD_TESTS` (default: OFF) - Build unit tests
- `BUILD_SHARED_LIBS` (default: ON) - Build as shared library

Example:
```bash
cmake -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON ..
```

## Quick Start

### Configuration

The SDK uses a modular configuration system with separate concerns for authentication, SQL settings, and connection pooling. The Builder pattern provides a clean API for constructing clients.

#### Configuration Structure

The SDK separates configuration into four distinct concerns:

- **`AuthConfig`**: Core authentication (host, token, timeout) - shared across all Databricks features
- **`SQLConfig`**: SQL-specific settings (http_path, ODBC driver name)
- **`PoolingConfig`**: Optional connection pooling settings (enabled, min/max connections)
- **`RetryConfig`**: Optional automatic retry settings (enabled, max attempts, backoff strategy)

This modular design allows you to:
- Share `AuthConfig` across different Databricks service clients (SQL, Workspace, Delta, etc.)
- Configure only what you need
- Mix automatic and explicit configuration

#### Option 1: Automatic Configuration (Recommended)

The SDK automatically loads configuration from `~/.databrickscfg` or environment variables:

```cpp
#include <databricks/client.h>

int main() {
    // Load from ~/.databrickscfg or environment variables
    auto client = databricks::Client::Builder()
        .with_environment_config()
        .build();
    
    auto results = client.query("SELECT * FROM my_table LIMIT 10");
    
    return 0;
}
```

**Configuration Precedence** (highest to lowest):
1. Profile file (`~/.databrickscfg` with `[DEFAULT]` section) - if complete, used exclusively
2. Environment variables (`DATABRICKS_HOST`, `DATABRICKS_TOKEN`, `DATABRICKS_HTTP_PATH`) - only as fallback

#### Option 2: Profile File

Create `~/.databrickscfg`:

```ini
[DEFAULT]
host = https://my-workspace.databricks.com
token = dapi1234567890abcdef
http_path = /sql/1.0/warehouses/abc123
# Alternative key name also supported:
# sql_http_path = /sql/1.0/warehouses/abc123

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

#### Option 3: Environment Variables Only

```bash
export DATABRICKS_HOST="https://my-workspace.databricks.com"
export DATABRICKS_TOKEN="dapi1234567890abcdef"
export DATABRICKS_HTTP_PATH="/sql/1.0/warehouses/abc123"
export DATABRICKS_TIMEOUT=120  # Optional

# Alternative variable names also supported:
# DATABRICKS_SERVER_HOSTNAME, DATABRICKS_ACCESS_TOKEN, DATABRICKS_SQL_HTTP_PATH
```

#### Option 4: Manual Configuration

```cpp
#include <databricks/client.h>

int main() {
    // Configure authentication
    databricks::AuthConfig auth;
    auth.host = "https://my-workspace.databricks.com";
    auth.token = "dapi1234567890abcdef";
    auth.timeout_seconds = 60;

    // Configure SQL settings
    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/abc123";
    sql.odbc_driver_name = "Simba Spark ODBC Driver";

    // Build client
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .build();

    // Execute a query
    auto results = client.query("SELECT * FROM my_table LIMIT 10");

    return 0;
}
```

### Async Connection (Non-blocking)

```cpp
#include <databricks/client.h>

int main() {
    // Build client without auto-connecting
    auto client = databricks::Client::Builder()
        .with_environment_config()
        .with_auto_connect(false)
        .build();

    // Start connection asynchronously
    auto connect_future = client.connect_async();

    // Do other work while connecting...

    // Wait for connection before querying
    connect_future.wait();
    auto results = client.query("SELECT current_timestamp()");

    return 0;
}
```

### Connection Pooling (High Performance)

```cpp
#include <databricks/client.h>

int main() {
    // Configure pooling
    databricks::PoolingConfig pooling;
    pooling.enabled = true;
    pooling.min_connections = 2;
    pooling.max_connections = 10;

    // Build client with pooling
    auto client = databricks::Client::Builder()
        .with_environment_config()
        .with_pooling(pooling)
        .build();

    // Query as usual - connections acquired/released automatically
    auto results = client.query("SELECT * FROM my_table");

    return 0;
}
```

**Note**: Multiple Clients with the same config automatically share the same pool!

### Automatic Retry Logic (Reliability)

The SDK includes automatic retry logic with exponential backoff for transient failures:

```cpp
#include <databricks/client.h>

int main() {
    // Configure retry behavior
    databricks::RetryConfig retry;
    retry.enabled = true;                 // Enable retries (default: true)
    retry.max_attempts = 5;               // Retry up to 5 times (default: 3)
    retry.initial_backoff_ms = 200;       // Start with 200ms delay (default: 100ms)
    retry.backoff_multiplier = 2.0;       // Double delay each retry (default: 2.0)
    retry.max_backoff_ms = 10000;         // Cap at 10 seconds (default: 10000ms)
    retry.retry_on_timeout = true;        // Retry timeout errors (default: true)
    retry.retry_on_connection_lost = true;// Retry connection errors (default: true)

    // Build client with retry configuration
    auto client = databricks::Client::Builder()
        .with_environment_config()
        .with_retry(retry)
        .build();

    // Queries automatically retry on transient errors
    auto results = client.query("SELECT * FROM my_table");

    return 0;
}
```

**Retry Features:**
- **Exponential backoff** with jitter to prevent thundering herd
- **Intelligent error classification** - only retries transient errors:
  - Connection timeouts and network errors
  - Server unavailability (503, 502, 504)
  - Rate limiting (429 Too Many Requests)
- **Non-retryable errors** fail immediately:
  - Authentication failures
  - SQL syntax errors
  - Permission denied errors
- **Enabled by default** with sensible defaults
- **Works with connection pooling** for maximum reliability

**Disable Retries (if needed):**
```cpp
databricks::RetryConfig no_retry;
no_retry.enabled = false;

auto client = databricks::Client::Builder()
    .with_environment_config()
    .with_retry(no_retry)
    .build();
```

### Mixing Configuration Approaches

The Builder pattern allows you to mix automatic and explicit configuration:

```cpp
// Load auth from environment, but customize pooling
databricks::PoolingConfig pooling;
pooling.enabled = true;
pooling.max_connections = 20;

auto client = databricks::Client::Builder()
    .with_environment_config()  // Load auth + SQL from environment
    .with_pooling(pooling)       // Override pooling settings
    .build();
```

Or load auth separately from SQL settings:

```cpp
// Load auth from profile, SQL from environment
databricks::AuthConfig auth = databricks::AuthConfig::from_profile("production");

databricks::SQLConfig sql;
sql.http_path = std::getenv("CUSTOM_HTTP_PATH");

auto client = databricks::Client::Builder()
    .with_auth(auth)
    .with_sql(sql)
    .build();
```

### Accessing Configuration

You can access the modular configuration from any client:

```cpp
auto client = databricks::Client::Builder()
    .with_environment_config()
    .build();

// Access configuration
const auto& auth = client.get_auth_config();
const auto& sql = client.get_sql_config();
const auto& pooling = client.get_pooling_config();

std::cout << "Connected to: " << auth.host << std::endl;
std::cout << "Using warehouse: " << sql.http_path << std::endl;
```

For a complete example, see `examples/basic/modular_config_example.cpp`.

## Running Examples

### Setup Configuration

Examples automatically load configuration from either:

**Option A: Profile File** (recommended for development)

Create `~/.databrickscfg`:
```ini
[DEFAULT]
host = https://your-workspace.databricks.com
token = your_databricks_token
http_path = /sql/1.0/warehouses/your_warehouse_id
# or: sql_http_path = /sql/1.0/warehouses/your_warehouse_id
```

**Option B: Environment Variables** (recommended for CI/CD)

```bash
export DATABRICKS_HOST="https://your-workspace.databricks.com"
export DATABRICKS_TOKEN="your_databricks_token"
export DATABRICKS_HTTP_PATH="/sql/1.0/warehouses/your_warehouse_id"
```

Or source a .env file:
```bash
set -a; source .env; set +a
```

**Note**: Profile configuration takes priority. Environment variables are used only as a fallback when no profile is configured.

### Run Examples

After building with `BUILD_EXAMPLES=ON`, examples are organized by feature:

### Basic Examples
```bash
# Simple SQL query
./examples/simple_query

# Jobs API - list jobs, get details, trigger runs
./examples/jobs_example
```

### Connection Pooling Examples
```bash
# Transparent connection pooling
./examples/transparent_pooling

# Benchmark pooling performance (measure actual speedup)
./examples/benchmark [num_queries]
```

### Async Examples
```bash
# Async operations
./examples/async_operations

# Combined pool + async (best performance)
./examples/pool_async_combined
```

### Testing Pooling Performance

To measure the actual performance improvement from connection pooling:

```bash
cd build
./examples/benchmark 10
```

This runs 10 queries with and without pooling, showing:
- Total time comparison
- Per-query time breakdown
- Speedup factor (typically 2-10x faster)
- Connection overhead estimation

Example output:
```
Without pooling: 5234ms total
With pooling:    892ms total

Time saved:      4342ms (83.0% faster)
Speedup:         5.87x
```

## Performance Considerations

### Connection Pooling Benefits

Connection pooling eliminates the overhead of creating new ODBC connections for each query:

- **Without pooling**: 500-2000ms per query (includes connection time)
- **With pooling**: 1-50ms per query (connection reused)
- **Recommended**: Use pooling for applications making multiple queries

### Async Operations Benefits

Async operations reduce perceived latency by performing work in the background:

- **Async connect**: Start connecting while doing other initialization
- **Async query**: Execute multiple queries concurrently
- **Combined with pooling**: Maximum throughput for concurrent workloads

### Best Practices

1. **Enable pooling** via `PoolingConfig` for applications making multiple queries
2. **Use async operations** when you can do other work while waiting
3. **Enable retry logic** (on by default) for production reliability against transient failures
4. **Combine pooling + retries** for maximum reliability and performance
5. **Size pools appropriately**: min = typical concurrent load, max = peak load
6. **Share configs**: Clients with identical configs automatically share pools
7. **Tune retry settings** based on your workload:
   - High-throughput: Lower `max_attempts` (2-3) to fail fast
   - Critical operations: Higher `max_attempts` (5-7) for maximum reliability
   - Rate-limited APIs: Increase `initial_backoff_ms` and `max_backoff_ms`

## Advanced Usage

### Jobs API

Interact with Databricks Jobs to automate and orchestrate data workflows:

```cpp
#include <databricks/jobs.h>
#include <databricks/config.h>

int main() {
    // Load auth configuration
    databricks::AuthConfig auth = databricks::AuthConfig::from_environment();

    // Create Jobs API client
    databricks::Jobs jobs(auth);

    // List all jobs
    auto job_list = jobs.list_jobs(25, 0);
    for (const auto& job : job_list) {
        std::cout << "Job: " << job.name
                  << " (ID: " << job.job_id << ")" << std::endl;
    }

    // Get specific job details
    auto job = jobs.get_job(123456789);
    std::cout << "Created by: " << job.creator_user_name << std::endl;

    // Trigger a job run with parameters
    std::map<std::string, std::string> params;
    params["date"] = "2024-01-01";
    params["environment"] = "production";

    uint64_t run_id = jobs.run_now(123456789, params);
    std::cout << "Started run: " << run_id << std::endl;

    return 0;
}
```

**Key Features:**
- **List jobs**: Paginated listing with limit/offset support
- **Get job details**: Retrieve full job configuration and metadata
- **Trigger runs**: Start jobs with optional notebook parameters
- **Type-safe IDs**: Uses `uint64_t` to correctly handle large job IDs
- **JSON parsing**: Built on `nlohmann/json` for reliable parsing

**API Compatibility:**
- Uses Jobs API 2.1 for full feature support including pagination
- Timestamps returned as Unix milliseconds (`uint64_t`)
- Automatic error handling with descriptive messages

For a complete example, see `examples/basic/jobs_example.cpp`.

### Direct ConnectionPool Management

For advanced users who need fine-grained control over connection pools:

```cpp
#include <databricks/connection_pool.h>

// Build config for pool
databricks::AuthConfig auth;
auth.host = "https://my-workspace.databricks.com";
auth.token = "dapi1234567890abcdef";

databricks::SQLConfig sql;
sql.http_path = "/sql/1.0/warehouses/abc123";

// Create and manage pool explicitly
databricks::ConnectionPool pool(auth, sql, 2, 10);
pool.warm_up();

// Acquire connections manually
{
    auto pooled_conn = pool.acquire();
    auto results = pooled_conn->query("SELECT...");
} // Connection returns to pool

// Monitor pool
auto stats = pool.get_stats();
std::cout << "Available: " << stats.available_connections << std::endl;
```

**Note**: Most users should use the Builder with `PoolingConfig` instead of direct pool management.

## Documentation

The SDK includes comprehensive API documentation generated from code comments using Doxygen.

### 📚 View Online Documentation

**Live Documentation**: [https://calvinjmin.github.io/databricks-sdk-cpp/](https://calvinjmin.github.io/databricks-sdk-cpp/)

The documentation is automatically built and published via GitHub Actions whenever changes are pushed to the `main` branch.

### Generate Documentation Locally

```bash
# Install Doxygen
brew install doxygen  # macOS
# or: sudo apt-get install doxygen  # Linux

# Generate docs (creates docs/html/)
doxygen Doxyfile

# View in browser
open docs/html/index.html  # macOS
# or: xdg-open docs/html/index.html  # Linux
```

### Documentation Features

The generated documentation includes:

- **Complete API Reference**: All public classes, methods, and structs with detailed descriptions
- **README Integration**: Full README displayed as the main landing page
- **Code Examples**: Inline examples from header comments
- **Jobs API Documentation**: Full reference for `databricks::Jobs`, `Job`, and `JobRun` types
- **SQL Client Documentation**: Complete `databricks::Client` API reference
- **Connection Pooling**: `databricks::ConnectionPool` and configuration types
- **Source Browser**: Browse source code with syntax highlighting
- **Search Functionality**: Quick search across all documentation
- **Cross-references**: Navigate between related classes and methods

### Quick Links (After Generation)

- **Main Page**: `docs/html/index.html` - README and getting started
- **Classes**: `docs/html/annotated.html` - All classes and structs
- **Jobs API**: `docs/html/classdatabricks_1_1_jobs.html` - Jobs API reference
- **Client API**: `docs/html/classdatabricks_1_1_client.html` - SQL client reference
- **Files**: `docs/html/files.html` - Browse by file

### Example: Viewing Jobs API Docs

```bash
# Generate and open Jobs API documentation
doxygen Doxyfile
open docs/html/classdatabricks_1_1_jobs.html
```

The documentation is automatically generated from the inline comments in header files, ensuring it stays synchronized with the code.

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues and questions, please open an issue on the GitHub repository.
