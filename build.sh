#!/bin/bash
# Kyros Build Script
#
# Usage:
#   ./build.sh [build-type] [options]
#
# Build types:
#   Release  - Optimized build (default)
#   Debug    - Debug build with symbols
#   Coverage - Coverage build with instrumentation
#
# Options:
#   --test       - Run tests after building
#   --coverage   - Generate coverage report (requires Coverage build)
#   --clean      - Clean build directory before building
#   --daemon     - Enable daemon build (requires SQLite3)
#   --help       - Show this help message

set -e

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
RUN_TESTS=false
GEN_COVERAGE=false
CLEAN_BUILD=false
BUILD_DAEMON=OFF

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        Release|Debug|Coverage)
            BUILD_TYPE="$1"
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --coverage)
            GEN_COVERAGE=true
            BUILD_TYPE="Coverage"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --daemon)
            BUILD_DAEMON=ON
            shift
            ;;
        --help)
            grep "^#" "$0" | grep -v "#!/bin/bash" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Run './build.sh --help' for usage information"
            exit 1
            ;;
    esac
done

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║       Building Kyros ($BUILD_TYPE)       ${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${GREEN}→ Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_DAEMON="$BUILD_DAEMON" \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo ""
echo -e "${GREEN}→ Building...${NC}"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo -e "${GREEN}✓ Build complete!${NC}"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║           Running Tests                ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
    echo ""
    ctest --output-on-failure --verbose
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
fi

# Generate coverage report if requested
if [ "$GEN_COVERAGE" = true ]; then
    if [ "$BUILD_TYPE" != "Coverage" ]; then
        echo -e "${YELLOW}Warning: Coverage report requires Coverage build type${NC}"
        echo "Skipping coverage generation"
    else
        echo ""
        echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
        echo -e "${BLUE}║      Generating Coverage Report        ║${NC}"
        echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
        echo ""
        cmake --build . --target coverage
        echo ""
        echo -e "${GREEN}✓ Coverage report generated!${NC}"
        echo -e "  View report: ${BLUE}$BUILD_DIR/coverage/html/index.html${NC}"
    fi
fi

# Summary
echo ""
echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║              Summary                    ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "  Binary:      ${GREEN}$BUILD_DIR/kyros${NC}"
echo -e "  Build type:  ${YELLOW}$BUILD_TYPE${NC}"
echo ""
echo -e "${YELLOW}Quick commands:${NC}"
echo -e "  Run tests:      ${BLUE}cd $BUILD_DIR && ctest --output-on-failure${NC}"
echo -e "  Test specific:  ${BLUE}cd $BUILD_DIR && ctest -R test_name${NC}"
echo -e "  List tests:     ${BLUE}cd $BUILD_DIR && ctest -N${NC}"
if [ "$BUILD_TYPE" = "Coverage" ]; then
    echo -e "  Coverage:       ${BLUE}cd $BUILD_DIR && cmake --build . --target coverage${NC}"
fi
echo -e "  Install:        ${BLUE}cd $BUILD_DIR && sudo cmake --install .${NC}"
echo ""
