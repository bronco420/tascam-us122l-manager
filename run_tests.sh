#!/bin/bash
# ============================================================================
# Tascam US-122L Manager - Run Tests
# ============================================================================

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Test functions
run_test() {
    local test_name="$1"
    local test_command="$2"

    echo -e "${YELLOW}[TEST]${NC} $test_name..."

    if eval "$test_command"; then
        echo -e "${GREEN}[PASS]${NC} $test_name"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}[FAIL]${NC} $test_name"
        ((TESTS_FAILED++))
    fi
}

# Check dependencies
check_dependencies() {
    echo -e "${YELLOW}[CHECK]${NC} Checking dependencies..."

    local missing=0

    if ! command -v cmake &>/dev/null; then
        echo -e "${RED}[ERROR]${NC} cmake not found"
        missing=1
    fi

    if ! command -v ninja &>/dev/null && ! command -v make &>/dev/null; then
        echo -e "${RED}[ERROR]${NC} ninja or make not found"
        missing=1
    fi

    if ! pkg-config --exists Qt6Core Qt6Widgets Qt6Network; then
        echo -e "${RED}[ERROR]${NC} Qt6 not found"
        missing=1
    fi

    if ! pkg-config --exists jack; then
        echo -e "${RED}[ERROR]${NC} JACK not found"
        missing=1
    fi

    if ! pkg-config --exists libpipewire-0.3; then
        echo -e "${RED}[ERROR]${NC} PipeWire not found"
        missing=1
    fi

    if ! pkg-config --exists alsa; then
        echo -e "${RED}[ERROR]${NC} ALSA not found"
        missing=1
    fi

    if [ $missing -eq 1 ]; then
        echo -e "${YELLOW}[WARN]${NC} Some dependencies are missing"
        exit 1
    fi

    echo -e "${GREEN}[OK]${NC} All dependencies found"
}

# Build test
test_build() {
    echo -e "${YELLOW}[TEST]${NC} Building project..."

    mkdir -p build
    cd build

    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .. > /dev/null 2>&1
    cmake --build . -j$(nproc) > /dev/null 2>&1

    cd ..

    if [ -f "build/tascam-us122l-manager" ]; then
        echo -e "${GREEN}[PASS]${NC} Build successful"
        return 0
    else
        echo -e "${RED}[FAIL]${NC} Build failed"
        return 1
    fi
}

# Syntax check
test_syntax() {
    echo -e "${YELLOW}[TEST]${NC} Checking syntax..."

    local syntax_errors=0

    for file in src/**/*.cpp src/**/*.h; do
        if [ -f "$file" ]; then
            if ! g++ -std=c++17 -fsyntax-only -I. "$file" 2>/dev/null; then
                ((syntax_errors++))
            fi
        fi
    done

    if [ $syntax_errors -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} Syntax check passed"
        return 0
    else
        echo -e "${RED}[FAIL]${NC} Syntax check failed ($syntax_errors errors)"
        return 1
    fi
}

# Unit tests (placeholder)
test_unit() {
    echo -e "${YELLOW}[TEST]${NC} Running unit tests..."

    # TODO: Add actual unit tests
    echo -e "${GREEN}[SKIP]${NC} Unit tests not implemented yet"
    return 0
}

# Integration tests (placeholder)
test_integration() {
    echo -e "${YELLOW}[TEST]${NC} Running integration tests..."

    # TODO: Add actual integration tests
    echo -e "${GREEN}[SKIP]${NC} Integration tests not implemented yet"
    return 0
}

# Performance test
test_performance() {
    echo -e "${YELLOW}[TEST]${NC} Testing performance..."

    # TODO: Add performance tests
    echo -e "${GREEN}[SKIP]${NC} Performance tests not implemented yet"
    return 0
}

# Memory leak test
test_memory() {
    echo -e "${YELLOW}[TEST]${NC} Testing for memory leaks..."

    # TODO: Add memory leak detection
    echo -e "${GREEN}[SKIP]${NC} Memory leak tests not implemented yet"
    return 0
}

# Help
show_help() {
    cat << EOF
Tascam US-122L Manager - Test Runner

Usage: ./run_tests.sh [OPTIONS]

Options:
  --all           Run all tests (default)
  --build         Test build
  --syntax        Test syntax
  --unit          Run unit tests
  --integration   Run integration tests
  --performance   Test performance
  --memory        Test for memory leaks
  --help          Show this help message

Examples:
  ./run_tests.sh                Run all tests
  ./run_tests.sh --build        Test build only
  ./run_tests.sh --syntax       Test syntax only

EOF
}

# Main
main() {
    echo "========================================"
    echo "  Tascam US-122L Manager - Test Runner"
    echo "========================================"
    echo ""

    # Check dependencies first
    check_dependencies

    # Parse arguments
    TEST_ALL=true
    TEST_BUILD=false
    TEST_SYNTAX=false
    TEST_UNIT=false
    TEST_INTEGRATION=false
    TEST_PERFORMANCE=false
    TEST_MEMORY=false

    for arg in "$@"; do
        case $arg in
            --all)
                TEST_ALL=true
                ;;
            --build)
                TEST_BUILD=true
                TEST_ALL=false
                ;;
            --syntax)
                TEST_SYNTAX=true
                TEST_ALL=false
                ;;
            --unit)
                TEST_UNIT=true
                TEST_ALL=false
                ;;
            --integration)
                TEST_INTEGRATION=true
                TEST_ALL=false
                ;;
            --performance)
                TEST_PERFORMANCE=true
                TEST_ALL=false
                ;;
            --memory)
                TEST_MEMORY=true
                TEST_ALL=false
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
        esac
    done

    # Run tests
    if [ "$TEST_ALL" = true ]; then
        run_test "Build" "test_build"
        run_test "Syntax" "test_syntax"
        run_test "Unit Tests" "test_unit"
        run_test "Integration Tests" "test_integration"
        run_test "Performance Tests" "test_performance"
        run_test "Memory Tests" "test_memory"
    else
        [ "$TEST_BUILD" = true ] && run_test "Build" "test_build"
        [ "$TEST_SYNTAX" = true ] && run_test "Syntax" "test_syntax"
        [ "$TEST_UNIT" = true ] && run_test "Unit Tests" "test_unit"
        [ "$TEST_INTEGRATION" = true ] && run_test "Integration Tests" "test_integration"
        [ "$TEST_PERFORMANCE" = true ] && run_test "Performance Tests" "test_performance"
        [ "$TEST_MEMORY" = true ] && run_test "Memory Tests" "test_memory"
    fi

    # Summary
    echo ""
    echo "========================================"
    echo "  Test Summary"
    echo "========================================"
    echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    echo ""

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}[SUCCESS]${NC} All tests passed!"
        exit 0
    else
        echo -e "${RED}[FAILURE]${NC} Some tests failed!"
        exit 1
    fi
}

main "$@"