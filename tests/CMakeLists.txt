cmake_minimum_required(VERSION 3.16)

# Find required packages for testing
find_package(GTest REQUIRED)

# Include directories for tests
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_SOURCE_DIR}/src/core/common)
include_directories(${GTEST_INCLUDE_DIRS})

# Test sources - only basic working files
set(TEST_SOURCES
    unit/test_basic.cpp
)

# Create test executable
add_executable(smart_monitor_tests ${TEST_SOURCES})

# Link libraries (minimal for now)
target_link_libraries(smart_monitor_tests
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
    m
)

# Set test properties
set_target_properties(smart_monitor_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Compiler flags for tests
target_compile_options(smart_monitor_tests PRIVATE
    -Wall
    -Wextra
    -g
)

# Register tests with CTest
add_test(NAME BasicTests COMMAND smart_monitor_tests --gtest_filter="SanityTest.*")
add_test(NAME FunctionalityTests COMMAND smart_monitor_tests --gtest_filter="BasicFunctionalityTest.*")

# Set test properties
set_tests_properties(BasicTests PROPERTIES TIMEOUT 30)
set_tests_properties(FunctionalityTests PROPERTIES TIMEOUT 30)
