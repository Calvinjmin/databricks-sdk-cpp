#!/bin/bash

# ODBC Setup Checker for Databricks C++ SDK
# This script verifies that ODBC is properly configured for the SDK

set -e

echo "========================================"
echo "Databricks C++ SDK - ODBC Setup Checker"
echo "========================================"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_passed=0
check_failed=0

# Check 1: unixODBC installation
echo "1. Checking unixODBC installation..."
if command -v odbcinst &> /dev/null; then
    ODBC_VERSION=$(odbcinst --version 2>&1 | head -n 1)
    echo -e "   ${GREEN}✓${NC} unixODBC found: $ODBC_VERSION"
    ((check_passed++))
else
    echo -e "   ${RED}✗${NC} unixODBC not found"
    echo "   Install with: brew install unixodbc (macOS) or apt-get install unixodbc-dev (Linux)"
    ((check_failed++))
fi
echo ""

# Check 2: ODBC configuration files
echo "2. Checking ODBC configuration..."
if command -v odbcinst &> /dev/null; then
    echo "   Configuration locations:"
    odbcinst -j | while read -r line; do
        echo "      $line"
    done
    echo -e "   ${GREEN}✓${NC} ODBC configuration accessible"
    ((check_passed++))
else
    echo -e "   ${RED}✗${NC} Cannot check ODBC configuration (odbcinst not available)"
    ((check_failed++))
fi
echo ""

# Check 3: Installed ODBC drivers
echo "3. Checking installed ODBC drivers..."
if command -v odbcinst &> /dev/null; then
    DRIVERS=$(odbcinst -q -d 2>&1)
    if [ -n "$DRIVERS" ]; then
        echo "   Installed drivers:"
        echo "$DRIVERS" | while read -r line; do
            if [[ $line == *"Simba Spark"* ]]; then
                echo -e "      ${GREEN}$line${NC} ← Recommended for Databricks"
            else
                echo "      $line"
            fi
        done

        if echo "$DRIVERS" | grep -q "Simba Spark"; then
            echo -e "   ${GREEN}✓${NC} Simba Spark ODBC Driver found"
            ((check_passed++))
        else
            echo -e "   ${YELLOW}⚠${NC} Simba Spark ODBC Driver not found"
            echo "   Download from: https://www.databricks.com/spark/odbc-drivers-download"
            ((check_failed++))
        fi
    else
        echo -e "   ${RED}✗${NC} No ODBC drivers found"
        echo "   Install Simba Spark ODBC Driver from: https://www.databricks.com/spark/odbc-drivers-download"
        ((check_failed++))
    fi
else
    echo -e "   ${RED}✗${NC} Cannot check drivers (odbcinst not available)"
    ((check_failed++))
fi
echo ""

# Check 4: Library paths (macOS specific)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "4. Checking library paths (macOS)..."
    LIBODBC_PATHS=(
        "/opt/homebrew/lib/libodbc.dylib"
        "/usr/local/lib/libodbc.dylib"
    )

    found_libodbc=false
    for path in "${LIBODBC_PATHS[@]}"; do
        if [ -f "$path" ]; then
            echo -e "   ${GREEN}✓${NC} Found libodbc at: $path"
            found_libodbc=true
            ((check_passed++))
            break
        fi
    done

    if ! $found_libodbc; then
        echo -e "   ${RED}✗${NC} libodbc.dylib not found in standard locations"
        echo "   Install with: brew install unixodbc"
        ((check_failed++))
    fi
    echo ""
fi

# Summary
echo "========================================"
echo "Summary"
echo "========================================"
echo -e "${GREEN}Passed: $check_passed${NC}"
echo -e "${RED}Failed: $check_failed${NC}"
echo ""

if [ $check_failed -eq 0 ]; then
    echo -e "${GREEN}✓ ODBC is properly configured!${NC}"
    echo "You should be able to build and run the Databricks C++ SDK."
    exit 0
else
    echo -e "${YELLOW}⚠ Some checks failed.${NC}"
    echo "Please resolve the issues above before building the SDK."
    echo ""
    echo "Quick setup guide:"
    echo "  1. Install unixODBC:"
    echo "     macOS:   brew install unixodbc"
    echo "     Linux:   sudo apt-get install unixodbc-dev"
    echo ""
    echo "  2. Download and install Simba Spark ODBC Driver:"
    echo "     https://www.databricks.com/spark/odbc-drivers-download"
    echo ""
    echo "  3. Verify driver installation:"
    echo "     odbcinst -q -d"
    exit 1
fi
