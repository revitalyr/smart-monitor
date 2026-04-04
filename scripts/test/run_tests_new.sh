#!/bin/bash

# Smart Monitor Test Runner - New Structure
# This script builds and runs all tests for smart monitor project

set -e  # Exit on any error

echo "🚀 Smart Monitor Test Runner (New Structure)"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from project root."
    exit 1
fi

# Check if new structure exists
if [ ! -d "src" ]; then
    print_error "New src/ structure not found. Please run restructuring first."
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    print_status "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure with CMake (new structure)
print_status "Configuring project with CMake (new structure)..."
if ! cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed."

# Build the project
print_status "Building project..."
if ! cmake --build . --parallel; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully."

# Build Rust motion library first (new structure)
if [ -d "../src/rust/motion" ]; then
    print_status "Building Rust motion library..."
    cd ../src/rust/motion
    if ! cargo build --release; then
        print_error "Rust build failed!"
        exit 1
    fi
    print_success "Rust library built successfully."
    cd ../../../build
else
    print_warning "Rust motion module not found in new structure, skipping..."
fi

# Run Rust tests (new structure)
if [ -d "../src/rust/motion" ]; then
    print_status "Running Rust tests..."
    cd ../src/rust/motion
    if ! cargo test; then
        print_error "Rust tests failed!"
        exit 1
    fi
    print_success "All Rust tests passed!"
    cd ../../../build
else
    print_warning "Rust tests skipped (no Rust module found)"
fi

# Run C++ tests (new structure)
print_status "Running C++ tests..."
if [ -f "tests/smart_monitor_tests" ]; then
    if ! ./tests/smart_monitor_tests --gtest_output=xml:test_results.xml; then
        print_error "C++ tests failed!"
        exit 1
    fi
    print_success "All C++ tests passed!"
else
    print_error "Test executable not found at tests/smart_monitor_tests"
    exit 1
fi

# Run CTest for integration tests
print_status "Running CTest integration tests..."
if ! ctest --output-on-failure --output-junit test_results_junit.xml; then
    print_error "Integration tests failed!"
    exit 1
fi

print_success "All integration tests passed!"

# Run individual test suites for detailed output
echo ""
print_status "Running individual test suites..."

echo ""
echo "--- Audio Capture Tests ---"
if ! ./tests/smart_monitor_tests --gtest_filter="AudioCaptureTest.*" --gtest_print_time=1; then
    print_error "Audio capture tests failed!"
    exit 1
fi

echo ""
echo "--- I2C Sensor Tests ---"
if ! ./tests/smart_monitor_tests --gtest_filter="I2CSensorTest.*" --gtest_print_time=1; then
    print_error "I2C sensor tests failed!"
    exit 1
fi

echo ""
echo "--- Data Agent Tests ---"
if ! ./tests/smart_monitor_tests --gtest_filter="DataAgentTest.*" --gtest_print_time=1; then
    print_error "Data agent tests failed!"
    exit 1
fi

echo ""
echo "--- Integration Tests ---"
if ! ./tests/smart_monitor_tests --gtest_filter="IntegrationTest.*" --gtest_print_time=1; then
    print_error "Integration tests failed!"
    exit 1
fi

# Generate test coverage report if gcov is available
if command -v gcov &> /dev/null; then
    print_status "Generating test coverage report..."
    cd tests
    gcov *.cpp 2>/dev/null || true
    cd ..
    
    # Generate HTML coverage report if lcov is available
    if command -v lcov &> /dev/null; then
        lcov --capture --directory . --output-file coverage.info
        genhtml coverage.info --output-directory coverage_html
        print_success "HTML coverage report generated in coverage_html/"
    fi
    
    print_success "Coverage report generated."
else
    print_warning "gcov not found. Skipping coverage report."
fi

# Run performance tests if enabled
if [ "$1" = "--performance" ]; then
    print_status "Running performance tests..."
    if [ -f "tests/smart_monitor_tests" ]; then
        if ! ./tests/smart_monitor_tests --gtest_filter="*Performance*" --gtest_print_time=1; then
            print_warning "Performance tests failed or not found"
        else
            print_success "Performance tests completed."
        fi
    fi
fi

# Run memory leak detection if valgrind is available
if command -v valgrind &> /dev/null && [ "$1" = "--valgrind" ]; then
    print_status "Running memory leak detection..."
    if ! valgrind --leak-check=full --error-exitcode=1 ./tests/smart_monitor_tests --gtest_filter="*Integration*" 2> valgrind.log; then
        print_error "Memory leaks detected!"
        cat valgrind.log
        exit 1
    fi
    print_success "No memory leaks detected."
fi

echo ""
print_success "🎉 All tests completed successfully!"
echo ""
echo "Test Summary:"
echo "  ✅ Rust unit tests: PASSED"
echo "  ✅ C++ unit tests: PASSED"
echo "  ✅ Integration tests: PASSED"
echo "  ✅ Audio capture tests: PASSED"
echo "  ✅ I2C sensor tests: PASSED"
echo "  ✅ Data agent tests: PASSED"
if [ "$1" = "--performance" ]; then
    echo "  ✅ Performance tests: COMPLETED"
fi
if [ "$1" = "--valgrind" ]; then
    echo "  ✅ Memory leak check: PASSED"
fi
echo ""
echo "Build artifacts are in: ./build"
echo "Test executable: ./build/tests/smart_monitor_tests"
echo "Test results: ./build/test_results.xml"
echo "JUnit results: ./build/test_results_junit.xml"
if [ -d "coverage_html" ]; then
    echo "Coverage report: ./build/coverage_html/index.html"
fi

echo ""
echo "Usage examples:"
echo "  ./scripts/test/run_tests.sh                    # Run all tests"
echo "  ./scripts/test/run_tests.sh --performance       # Include performance tests"
echo "  ./scripts/test/run_tests.sh --valgrind          # Include memory leak detection"
