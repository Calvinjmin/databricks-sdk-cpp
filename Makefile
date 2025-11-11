# Databricks C++ SDK Makefile
# This Makefile provides convenient wrappers around CMake commands
#
# VERSION MANAGEMENT:
# - Version is defined in CMakeLists.txt (line 3)
# - The following files are AUTO-GENERATED from .in templates:
#   * Doxyfile (from Doxyfile.in)
#   * vcpkg.json (from vcpkg.json.in)
#   * include/databricks/version.h (from version.h.in)
# - To update version: ./scripts/update_version.sh <version>
#   or manually edit CMakeLists.txt and run 'make configure'

# Build configuration
BUILD_DIR := build
CMAKE := cmake
CMAKE_FLAGS := -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Default target
.PHONY: all
all: build

# Configure the build
.PHONY: configure
configure:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) ..

# Build the project
.PHONY: build
build: configure
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Build with examples
.PHONY: build-examples
build-examples:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DBUILD_EXAMPLES=ON ..
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Build with tests
.PHONY: build-tests
build-tests:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DBUILD_TESTS=ON ..
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Build everything (examples + tests)
.PHONY: build-all
build-all:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON ..
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Run tests
.PHONY: test
test: build-tests
	cd $(BUILD_DIR) && ctest --output-on-failure

# Run only tests related to modified files (based on git status)
.PHONY: test-new
test-new: build-tests
	@echo "Running tests for modified files..."
	@MODIFIED_FILES=$$(git status --short | grep -E '^\s*M.*\.(cpp|h)' | awk '{print $$2}' | grep -E 'test|jobs|compute|unity' || true); \
	if [ -z "$$MODIFIED_FILES" ]; then \
		echo "No modified test files detected. Running all tests..."; \
		cd $(BUILD_DIR)/tests && ./unit_tests; \
	else \
		echo "Modified files: $$MODIFIED_FILES"; \
		if echo "$$MODIFIED_FILES" | grep -q "jobs"; then \
			echo "Running Jobs tests..."; \
			cd $(BUILD_DIR)/tests && ./unit_tests --gtest_filter='*Job*'; \
		elif echo "$$MODIFIED_FILES" | grep -q "compute"; then \
			echo "Running Compute tests..."; \
			cd $(BUILD_DIR)/tests && ./unit_tests --gtest_filter='*Compute*:*Cluster*'; \
		elif echo "$$MODIFIED_FILES" | grep -q "unity"; then \
			echo "Running Unity Catalog tests..."; \
			cd $(BUILD_DIR)/tests && ./unit_tests --gtest_filter='*UnityCatalog*'; \
		else \
			echo "Running all tests for safety..."; \
			cd $(BUILD_DIR)/tests && ./unit_tests; \
		fi \
	fi

# Run tests with custom filter
# Usage: make test-filter FILTER='JobsApiTest.*'
.PHONY: test-filter
test-filter: build-tests
	@if [ -z "$(FILTER)" ]; then \
		echo "Usage: make test-filter FILTER='pattern'"; \
		echo "Example: make test-filter FILTER='JobsApiTest.*'"; \
		echo "Example: make test-filter FILTER='*Cancel*'"; \
		exit 1; \
	fi
	@echo "Running tests matching filter: $(FILTER)"
	cd $(BUILD_DIR)/tests && ./unit_tests --gtest_filter='$(FILTER)'

# List all available tests
.PHONY: test-list
test-list: build-tests
	@echo "Available test cases:"
	cd $(BUILD_DIR)/tests && ./unit_tests --gtest_list_tests

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Note: Regenerating version-dependent files (Doxyfile, vcpkg.json, version.h)..."
	@echo "Run 'make configure' or 'make build' to regenerate them."

# Install the library
.PHONY: install
install: build
	cd $(BUILD_DIR) && $(CMAKE) --install .

# Uninstall (if supported)
.PHONY: uninstall
uninstall:
	cd $(BUILD_DIR) && $(CMAKE) --build . --target uninstall 2>/dev/null || \
		echo "Uninstall target not available"

# Build in release mode
.PHONY: release
release:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release ..
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Build in debug mode
.PHONY: debug
debug:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug ..
	cd $(BUILD_DIR) && $(CMAKE) --build .

# Rebuild from scratch
.PHONY: rebuild
rebuild: clean build

# Format code (if clang-format is available)
.PHONY: format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		find src include examples tests -type f \( -name "*.cpp" -o -name "*.h" \) \
			-exec clang-format -i {} +; \
		echo "Code formatted successfully"; \
	else \
		echo "clang-format not found. Please install it to use this target."; \
	fi

# Generate documentation with Doxygen
.PHONY: docs
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
		if [ ! -f Doxyfile ]; then \
			echo "Doxyfile not found. Generating from template..."; \
			$(MAKE) configure; \
		fi; \
		doxygen Doxyfile; \
		echo "Documentation generated in docs/html/index.html"; \
		echo "Run 'make docs-open' to view in browser"; \
	else \
		echo "Doxygen not found. Install with: brew install doxygen"; \
	fi

# Open documentation in browser
.PHONY: docs-open
docs-open:
	@if [ -f docs/html/index.html ]; then \
		open docs/html/index.html; \
	else \
		echo "Documentation not found. Run 'make docs' first."; \
	fi

# Clean documentation
.PHONY: clean-docs
clean-docs:
	rm -rf docs/html docs/latex

# Update version (interactive)
.PHONY: update-version
update-version:
	@if [ -z "$(VERSION)" ]; then \
		echo "Usage: make update-version VERSION=x.y.z"; \
		echo "Example: make update-version VERSION=0.3.0"; \
		echo ""; \
		echo "Or use the interactive script:"; \
		echo "  ./scripts/update_version.sh <version>"; \
	else \
		./scripts/update_version.sh $(VERSION); \
	fi

# Run pooling benchmark
.PHONY: benchmark
benchmark: build-examples
	@echo "Running connection pooling benchmark..."
	@cd $(BUILD_DIR) && ./examples/benchmark 10

# Help target
.PHONY: help
help:
	@echo "Databricks C++ SDK - Available targets:"
	@echo ""
	@echo "Building:"
	@echo "  make               - Build the project (default)"
	@echo "  make build         - Build the project"
	@echo "  make configure     - Run CMake configuration"
	@echo "  make build-examples - Build with examples"
	@echo "  make build-tests   - Build with tests"
	@echo "  make build-all     - Build with examples and tests"
	@echo "  make clean         - Remove build artifacts"
	@echo ""
	@echo "Testing:"
	@echo "  make test          - Run all tests"
	@echo "  make test-new      - Run tests for modified files (smart)"
	@echo "  make test-filter FILTER='pattern' - Run tests matching pattern"
	@echo "  make test-list     - List all available test cases"
	@echo "  make benchmark     - Run connection pooling benchmark"
	@echo ""
	@echo "Documentation:"
	@echo "  make docs          - Generate API documentation"
	@echo "  make docs-open     - Open documentation in browser"
	@echo "  make clean-docs    - Remove generated documentation"
	@echo ""
	@echo "Installation:"
	@echo "  make install       - Install the library"
	@echo "  make uninstall     - Uninstall the library"
	@echo ""
	@echo "Development:"
	@echo "  make release       - Build in release mode"
	@echo "  make debug         - Build in debug mode"
	@echo "  make rebuild       - Clean and rebuild"
	@echo "  make format        - Format code with clang-format"
	@echo ""
	@echo "Version Management:"
	@echo "  make update-version VERSION=x.y.z - Update SDK version"
	@echo "  ./scripts/update_version.sh <ver> - Interactive version update"
	@echo ""
	@echo "Other:"
	@echo "  make help          - Show this help message"
