# Tests

This directory contains tests for the Databricks C++ SDK, organized by test type.

## Structure

```
tests/
├── unit/                    # Unit tests (fast, no external dependencies)
│   └── test_client.cpp      # Client & Config tests (Google Test)
├── helpers/                 # Test utilities and helpers
│   └── test_utils.h         # Common test utilities
└── CMakeLists.txt           # Test configuration
```

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

All tests use Google Test framework. Add new tests to `unit/test_client.cpp`:

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

### Available Assertions

- `EXPECT_TRUE(condition)` / `EXPECT_FALSE(condition)`
- `EXPECT_EQ(expected, actual)` / `EXPECT_NE(expected, actual)`
- `EXPECT_THROW(statement, exception_type)`
- See [Google Test documentation](https://google.github.io/googletest/) for more
