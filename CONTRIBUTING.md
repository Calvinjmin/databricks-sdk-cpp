# Contributing to Databricks C++ SDK

Thank you for your interest in contributing to the Databricks C++ SDK! This document provides guidelines and instructions for contributing.

## 🌟 Ways to Contribute

- **Report Bugs**: Found a bug? Open an issue with details to reproduce it
- **Suggest Features**: Have an idea? Open an issue to discuss it
- **Improve Documentation**: Fix typos, clarify instructions, add examples
- **Write Code**: Fix bugs, implement features, add tests
- **Review Pull Requests**: Help review and test others' contributions

## 🚀 Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+)
- CMake 3.14 or higher
- ODBC Driver Manager (unixODBC on Linux/macOS)
- [Simba Spark ODBC Driver](https://www.databricks.com/spark/odbc-drivers-download)

### Setup Development Environment

1. **Fork and Clone**
   ```bash
   git clone https://github.com/YOUR_USERNAME/databricks-sdk-cpp.git
   cd databricks-sdk-cpp
   ```

2. **Build the Project**
   ```bash
   make build-all
   ```

3. **Run Tests**
   ```bash
   make test
   ```

4. **Verify ODBC Setup**
   ```bash
   ./scripts/check_odbc_setup.sh
   ```

## 📝 Development Workflow

### 1. Create a Branch

```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/issue-number-description
```

Branch naming conventions:
- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation changes
- `refactor/` - Code refactoring
- `test/` - Test additions or changes

### 2. Make Your Changes

Follow the coding standards below and make sure to:
- Write clear, self-documenting code
- Add comments for complex logic
- Update documentation if needed
- Add tests for new functionality

### 3. Test Your Changes

```bash
# Run all tests
make test

# Build examples to verify they still compile
make build-examples

# Run specific test
cd build && ./tests/unit_tests --gtest_filter=ClientTest.*
```

### 4. Commit Your Changes

Write clear commit messages following this format:

```
<type>: <subject>

<body>

<footer>
```

**Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation changes
- `style:` Code style changes (formatting, etc.)
- `refactor:` Code refactoring
- `test:` Test changes
- `chore:` Build process or auxiliary tool changes

**Example:**
```
feat: add support for Unity Catalog volumes API

Implement VolumeClient class with list, create, and delete operations.
Includes full error handling and retry logic.

Closes #123
```

### 5. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

Then create a pull request on GitHub with:
- Clear description of changes
- Link to related issues
- Screenshots/examples if UI or behavior changes
- Checklist completion (see PR template)

## 🎨 Code Style

### C++ Style Guide

We use `.clang-format` to enforce consistent style:

```bash
make format
```

**Key conventions:**
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 120 characters maximum
- **Naming**:
  - `snake_case` for variables, functions, and file names
  - `PascalCase` for classes and structs
  - `SCREAMING_SNAKE_CASE` for constants
- **Braces**: K&R style (opening brace on same line)
- **Pointers/References**: `Type* ptr` and `Type& ref` (left alignment)

**Example:**
```cpp
namespace databricks {

class MyClass {
public:
    MyClass(const std::string& name);
    
    bool process_data(const std::vector<int>& data);
    
private:
    std::string name_;
    int count_ = 0;
};

} // namespace databricks
```

### Header Guards

Use `#pragma once` for all new headers:

```cpp
// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#pragma once

namespace databricks {
// ... your code
}
```

### Documentation

Use Doxygen comments for all public APIs:

```cpp
/**
 * @brief Brief description of the function
 *
 * Detailed description with usage examples if needed.
 *
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * @throws std::runtime_error if something goes wrong
 *
 * @code
 * auto client = Client::Builder().build();
 * auto result = client.query("SELECT * FROM table");
 * @endcode
 */
std::vector<std::string> query(const std::string& sql, int limit);
```

## 🧪 Testing Guidelines

### Writing Tests

- Use Google Test framework (already integrated)
- Place tests in `tests/unit/` directory
- Name test files: `test_<component>.cpp`
- Use descriptive test names: `TEST(ComponentTest, MethodNameWithScenario)`

**Example:**
```cpp
#include <gtest/gtest.h>
#include <databricks/core/client.h>

TEST(ClientTest, QueryWithValidParameters) {
    // Arrange
    databricks::AuthConfig auth;
    auth.host = "https://test.databricks.com";
    auth.set_token("test_token");
    
    databricks::SQLConfig sql;
    sql.http_path = "/sql/1.0/warehouses/test";
    
    auto client = databricks::Client::Builder()
        .with_auth(auth)
        .with_sql(sql)
        .build();
    
    // Act & Assert
    EXPECT_EQ(client.get_auth_config().host, "https://test.databricks.com");
}
```

### Test Coverage

- Aim for >80% code coverage for new features
- Test both success and failure paths
- Test edge cases and boundary conditions
- Use mocks for external dependencies (see `tests/mocks/`)

## 🔍 Code Review Process

### What Reviewers Look For

1. **Functionality**: Does it work as intended?
2. **Tests**: Are there adequate tests?
3. **Documentation**: Is it well documented?
4. **Style**: Does it follow project conventions?
5. **Performance**: Any performance implications?
6. **Security**: Any security concerns?
7. **Compatibility**: Works on Linux/macOS?

### Responding to Feedback

- Be open to feedback - it makes the code better!
- Respond to all comments (even if just to acknowledge)
- Make requested changes or explain why not
- Mark conversations as resolved once addressed
- Update your PR based on feedback

## 🏷️ Issue Labels

When creating issues, use appropriate labels:

- `bug` - Something isn't working
- `feature` - New feature request
- `documentation` - Documentation improvements
- `good first issue` - Good for newcomers
- `help wanted` - Community help needed
- `question` - Further information requested
- `enhancement` - Improvement to existing feature
- `performance` - Performance related
- `security` - Security related

## 🎯 Good First Issues

New to the project? Look for issues labeled `good first issue`:

**Example Good First Issues:**
- Add missing documentation
- Fix typos or formatting
- Add example code
- Write additional tests
- Small bug fixes

## 📚 Additional Resources

### Project Documentation
- [README.md](README.md) - Project overview and usage
- [API Documentation](https://calvinjmin.github.io/databricks-sdk-cpp/) - Generated API docs
- [Tests README](tests/README.md) - Testing guide

### External Resources
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Databricks API Documentation](https://docs.databricks.com/dev-tools/api/latest/)
- [Google Test Documentation](https://google.github.io/googletest/)

## 💬 Communication

### Getting Help

- **Questions**: Open a GitHub Discussion or Issue with `question` label
- **Bugs**: Open an issue with reproduction steps
- **Security**: For security concerns, please open a private security advisory on GitHub (don't open public issues)

## 🏆 Recognition

Contributors are recognized in:
- GitHub contributors page
- Release notes
- Project documentation

Thank you for contributing to make this project better! 🎉

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.

