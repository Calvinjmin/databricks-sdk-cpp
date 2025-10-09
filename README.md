# Databricks C++ SDK

A C++ SDK for Databricks, providing an interface for interacting with Databricks services.

**Author**: Calvin Min (calvinjmin@gmail.com)

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
databricks::Client::Config config;
config.odbc_driver_name = "Your Driver Name Here"; // Must match driver name from odbcinst -q -d
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

## Building

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

The SDK supports multiple ways to configure credentials and connection details, with profile files taking precedence over environment variables.

#### Option 1: Automatic Configuration (Recommended)

The SDK automatically loads configuration from `~/.databrickscfg` or environment variables:

```cpp
#include <databricks/client.h>

int main() {
    // Load from ~/.databrickscfg or environment variables
    auto config = databricks::Client::Config::from_environment();
    
    databricks::Client client(config);
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
auto config = databricks::Client::Config::from_environment("production");
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
    // Configure manually
    databricks::Client::Config config;
    config.host = "https://my-workspace.databricks.com";
    config.token = "dapi1234567890abcdef";
    config.http_path = "/sql/1.0/warehouses/abc123";

    // Create client (lazy connection - connects on first query)
    databricks::Client client(config);

    // Execute a query
    auto results = client.query("SELECT * FROM my_table LIMIT 10");

    return 0;
}
```

### Async Connection (Non-blocking)

```cpp
#include <databricks/client.h>

int main() {
    databricks::Client::Config config;
    config.host = "https://my-workspace.databricks.com";
    config.token = "dapi1234567890abcdef";
    config.http_path = "/sql/1.0/warehouses/abc123";

    // Create client without auto-connect
    databricks::Client client(config, false);

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
    databricks::Client::Config config;
    config.host = "https://my-workspace.databricks.com";
    config.token = "dapi1234567890abcdef";
    config.http_path = "/sql/1.0/warehouses/abc123";

    // Enable pooling - that's it!
    config.enable_pooling = true;
    config.pool_min_connections = 2;
    config.pool_max_connections = 10;

    // Create client - pooling happens automatically
    databricks::Client client(config);

    // Query as usual - connections acquired/released automatically
    auto results = client.query("SELECT * FROM my_table");

    return 0;
}
```

**Note**: Multiple Clients with the same config automatically share the same pool!

## Using in Your Project

### CMake Integration

After installation:

```cmake
find_package(databricks_sdk REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE databricks_sdk::databricks_sdk)
```

### Manual Integration

Add the include directory and link against the library:

```cmake
target_include_directories(my_app PRIVATE /path/to/databricks-sdk-cpp/include)
target_link_libraries(my_app PRIVATE databricks_sdk)
```

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

1. **Enable pooling** via `config.enable_pooling = true` for applications making multiple queries
2. **Use async operations** when you can do other work while waiting
3. **Combine both** for maximum performance in concurrent scenarios
4. **Size pools appropriately**: min = typical concurrent load, max = peak load
5. **Share configs**: Clients with identical configs automatically share pools

## Advanced Usage

### Direct ConnectionPool Management

For advanced users who need fine-grained control over connection pools:

```cpp
#include <databricks/connection_pool.h>

// Create and manage pool explicitly
databricks::ConnectionPool pool(config, 2, 10);
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

**Note**: Most users should use `config.enable_pooling = true` instead of direct pool management.

## Documentation

Generate API documentation from code comments:

```bash
# Install Doxygen (macOS)
brew install doxygen

# Generate documentation
make docs

# Open documentation in browser
open docs/html/index.html
```

The documentation includes:
- API reference for all classes and methods
- Code examples from comments
- Class diagrams and inheritance trees
- Search functionality

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues and questions, please open an issue on the GitHub repository.
