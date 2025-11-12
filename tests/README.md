# Tests

This directory contains tests for the Databricks C++ SDK, organized by test type.

## Running Tests

### Build Tests

```bash
cd build
cmake -DBUILD_TESTS=ON ..
make
```

Or using the Makefile:
```bash
make build-tests
```

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

Or:
```bash
make test
```

### Run Specific Tests

**All unit tests:**
```bash
cd build
./unit_tests
```

**Run specific test:**
```bash
./unit_tests --gtest_filter=ClientTest.DefaultConstruction
```

## Test Types

### Unit Tests
- Fast, isolated tests
- No external dependencies
- Test individual components (Client, Config, etc.)
- Run on every build
- Built with Google Test framework

## Writing Tests

All tests use Google Test framework with Google Mock support for dependency injection.

### Basic Unit Tests

Add struct and configuration tests:

```cpp
#include <gtest/gtest.h>
#include <databricks/client.h>

TEST(ClientTest, YourTestName)
{
    // Arrange
    databricks::Client::Config config;
    config.host = "https://test.databricks.com";

    // Act
    databricks::Client client(config);

    // Assert
    EXPECT_EQ(client.get_config().host, "https://test.databricks.com");
}
```

### Mocking HTTP Requests

The SDK supports dependency injection for testing REST API clients without making real HTTP calls.

#### Using MockHttpClient

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/compute/compute.h>
#include <mock_http_client.h>

using ::testing::_;
using ::testing::Return;

TEST(ComputeTest, CreateClusterSuccess) {
    // Create mock HTTP client
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    // Setup expectations
    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response()));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    // Inject mock into Compute client
    databricks::Compute compute(mock_http);

    // Create cluster configuration
    databricks::Cluster config;
    config.cluster_name = "test-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 2;

    // Execute and verify
    bool result = compute.create_compute(config);
    EXPECT_TRUE(result);
}
```

#### Available Mock Response Helpers

The `MockHttpClient` provides helper methods for common responses:

- `success_response(body)` - HTTP 200 with custom body
- `bad_request_response(message)` - HTTP 400 Bad Request
- `unauthorized_response()` - HTTP 401 Unauthorized
- `not_found_response(resource)` - HTTP 404 Not Found
- `server_error_response()` - HTTP 500 Internal Server Error
- `cluster_created_response(cluster_id)` - Cluster creation success

#### Verifying Request Payloads

Capture and verify the JSON sent in requests:

```cpp
std::string captured_body;
EXPECT_CALL(*mock_http, post("/clusters/create", _))
    .WillOnce(::testing::DoAll(
        ::testing::SaveArg<1>(&captured_body),
        Return(databricks::test::MockHttpClient::success_response())
    ));

// ... execute operation ...

// Verify JSON payload
EXPECT_NE(captured_body.find("\"cluster_name\":\"test\""), std::string::npos);
```

#### Testing Error Scenarios

```cpp
EXPECT_CALL(*mock_http, post(_, _))
    .WillOnce(Return(databricks::test::MockHttpClient::server_error_response()));

EXPECT_CALL(*mock_http, check_response(_, _))
    .WillOnce(::testing::Throw(std::runtime_error("Server error")));

// Verify exception is thrown
EXPECT_THROW({
    compute.create_compute(config);
}, std::runtime_error);
```

### Available Assertions

- `EXPECT_TRUE(condition)` / `EXPECT_FALSE(condition)`
- `EXPECT_EQ(expected, actual)` / `EXPECT_NE(expected, actual)`
- `EXPECT_THROW(statement, exception_type)`
- `EXPECT_CALL(mock, method(args))` - Google Mock expectation
- See [Google Test documentation](https://google.github.io/googletest/) and [Google Mock documentation](https://google.github.io/googletest/gmock_for_dummies.html) for more
