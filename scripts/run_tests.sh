#!/bin/bash

# Smart Monitor Test Runner
# This script builds and runs all tests for the smart monitor project

set -e  # Exit on any error

echo "🚀 Smart Monitor Test Runner"
echo "============================"

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

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    print_status "Creating build directory..."
    mkdir -p build
fi

cd build

# Configure with CMake
print_status "Configuring project with CMake..."
if ! cmake -DCMAKE_BUILD_TYPE=Debug ..; then
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

# Build Rust motion library first
print_status "Building Rust motion library..."
cd ../firmware/rust_modules/motion
if ! cargo build --release; then
    print_error "Rust build failed!"
    exit 1
fi
print_success "Rust library built successfully."

cd ../../../build

# Run Rust tests
print_status "Running Rust tests..."
cd ../firmware/rust_modules/motion
if ! cargo test; then
    print_error "Rust tests failed!"
    exit 1
fi
print_success "All Rust tests passed!"

cd ../../../build

# Run C++ tests
print_status "Running C++ tests..."
if ! ctest --output-on-failure; then
    print_error "C++ tests failed!"
    exit 1
fi
print_success "All C++ tests passed!"

# Run individual test suites for detailed output
echo ""
print_status "Running individual test suites..."

echo ""
echo "--- V4L2 Capture Tests ---"
if ! ./firmware/cpp_core/smart_monitor_tests --gtest_filter="V4L2CaptureTest.*" --gtest_print_time=1; then
    print_error "V4L2 tests failed!"
    exit 1
fi

echo ""
echo "--- Rust Bridge Tests ---"
if ! ./firmware/cpp_core/smart_monitor_tests --gtest_filter="RustBridgeTest.*" --gtest_print_time=1; then
    print_error "Rust bridge tests failed!"
    exit 1
fi

echo ""
echo "--- HTTP Server Tests ---"
if ! ./firmware/cpp_core/smart_monitor_tests --gtest_filter="HTTPServerTest.*" --gtest_print_time=1; then
    print_error "HTTP server tests failed!"
    exit 1
fi

echo ""
echo "--- Integration Tests ---"
if ! ./firmware/cpp_core/smart_monitor_tests --gtest_filter="IntegrationTest.*" --gtest_print_time=1; then
    print_error "Integration tests failed!"
    exit 1
fi

# Generate test coverage report if gcov is available
if command -v gcov &> /dev/null; then
    print_status "Generating test coverage report..."
    cd firmware/cpp_core
    gcov *.cpp
    print_success "Coverage report generated."
    cd ../../..
else
    print_warning "gcov not found. Skipping coverage report."
fi

echo ""
print_success "🎉 All tests completed successfully!"
echo ""
echo "Test Summary:"
echo "  ✅ Rust unit tests: PASSED"
echo "  ✅ C++ unit tests: PASSED"
echo "  ✅ Integration tests: PASSED"
echo "  ✅ V4L2 Capture tests: PASSED"
echo "  ✅ Rust Bridge tests: PASSED"
echo "  ✅ HTTP Server tests: PASSED"
echo ""
echo "Build artifacts are in: ./build"
echo "Test executable: ./build/firmware/cpp_core/smart_monitor_tests"
