#!/bin/bash
#
# Version Update Helper Script
# Updates version in CMakeLists.txt and reminds about README.md
#
# Usage: ./scripts/update_version.sh <new_version>
# Example: ./scripts/update_version.sh 0.3.0
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -ne 1 ]; then
    echo -e "${RED}Error: Version number required${NC}"
    echo "Usage: $0 <version>"
    echo "Example: $0 0.3.0"
    exit 1
fi

NEW_VERSION="$1"

# Validate version format (basic semver check)
if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo -e "${RED}Error: Invalid version format${NC}"
    echo "Version must be in format: MAJOR.MINOR.PATCH (e.g., 0.3.0)"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

CMAKE_FILE="$PROJECT_ROOT/CMakeLists.txt"
README_FILE="$PROJECT_ROOT/README.md"

# Check if CMakeLists.txt exists
if [ ! -f "$CMAKE_FILE" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found at $CMAKE_FILE${NC}"
    exit 1
fi

# Get current version from project() line
CURRENT_VERSION=$(grep "project(" -A 1 "$CMAKE_FILE" | grep "VERSION" | sed 's/.*VERSION \([0-9.]*\).*/\1/')

echo -e "${GREEN}=== Databricks C++ SDK Version Update ===${NC}"
echo ""
echo "Current version: $CURRENT_VERSION"
echo "New version:     $NEW_VERSION"
echo ""

# Confirm
read -p "Continue with version update? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 0
fi

echo ""
echo -e "${YELLOW}Step 1: Updating CMakeLists.txt...${NC}"

# Update CMakeLists.txt (project VERSION line only)
sed -i.bak "/^project(/,/^)/ s/VERSION $CURRENT_VERSION/VERSION $NEW_VERSION/" "$CMAKE_FILE"
rm -f "$CMAKE_FILE.bak"

echo -e "${GREEN}✓ Updated CMakeLists.txt${NC}"

echo ""
echo -e "${YELLOW}Step 2: Regenerating version-dependent files...${NC}"

# Regenerate files
cd "$PROJECT_ROOT"
if [ -d "build" ]; then
    cd build
    cmake .. > /dev/null 2>&1
    cd ..
else
    mkdir -p build
    cd build
    cmake .. > /dev/null 2>&1
    cd ..
fi

echo -e "${GREEN}✓ Regenerated: version.h, Doxyfile, vcpkg.json${NC}"

echo ""
echo -e "${YELLOW}Step 3: Manual README.md update required${NC}"

# Check if README needs updating
if grep -q "v$CURRENT_VERSION" "$README_FILE"; then
    echo -e "${RED}⚠ README.md still references v$CURRENT_VERSION${NC}"
    echo ""
    echo "Please manually update the following in README.md:"
    echo "  Line 9: Change 'v$CURRENT_VERSION' to 'v$NEW_VERSION'"
    echo ""
    echo "  Find:    [v$CURRENT_VERSION]"
    echo "  Replace: [v$NEW_VERSION]"
    echo ""
fi

echo ""
echo -e "${GREEN}=== Version Update Summary ===${NC}"
echo ""
echo "✓ CMakeLists.txt:  $CURRENT_VERSION → $NEW_VERSION"
echo "✓ version.h:       Auto-generated"
echo "✓ Doxyfile:        Auto-generated"
echo "✓ vcpkg.json:      Auto-generated"
echo "⚠ README.md:       Needs manual update (line 9)"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Update README.md line 9 (if not done)"
echo "2. Review changes: git diff"
echo "3. Commit: git add -A && git commit -m \"Bump version to v$NEW_VERSION\""
echo "4. Tag:    git tag v$NEW_VERSION"
echo "5. Push:   git push && git push --tags"
echo ""
