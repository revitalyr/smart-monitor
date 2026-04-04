cmake_minimum_required(VERSION 3.16)
project(smart_monitor VERSION 1.0.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_WEBSOCKET "Build WebSocket server" ON)
option(BUILD_RUST "Build Rust components" OFF)
option(ENABLE_SIMULATION "Enable simulation mode" ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Try to find GStreamer (optional for now)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GSTREAMER QUIET gstreamer-1.0 gstreamer-app-1.0)
    if(GSTREAMER_FOUND)
        set(HAS_GSTREAMER TRUE)
        message(STATUS "GStreamer found: ${GSTREAMER_VERSION}")
    endif()
endif()

# Try to find OpenCV (optional for now)
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    set(HAS_OPENCV TRUE)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
endif()

# Testing packages
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core)

if(HAS_GSTREAMER)
    include_directories(${GSTREAMER_INCLUDE_DIRS})
endif()

if(HAS_OPENCV)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Add subdirectories based on what exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
    add_subdirectory(src/core)
endif()

if(BUILD_RUST AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/rust)
    add_subdirectory(src/rust)
endif()

# Web server components
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/web/api)
    add_subdirectory(src/web/api)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/web/protocol)
    add_subdirectory(src/web/protocol)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/web/webrtc)
    add_subdirectory(src/web/webrtc)
endif()

# Tests
if(BUILD_TESTS)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()

# Main executable (if it exists)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/core/main.c)
    set(SOURCES
        src/core/main.c
    )

    add_executable(smart_monitor_app ${SOURCES})

    # Link libraries (minimal version)
    set(LINK_LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        m
    )

    if(HAS_GSTREAMER)
        list(APPEND LINK_LIBS ${GSTREAMER_LIBRARIES})
    endif()

    if(HAS_OPENCV)
        list(APPEND LINK_LIBS ${OpenCV_LIBS})
    endif()

    # Try to link with core libraries if they exist
    if(TARGET smart_monitor_core)
        list(APPEND LINK_LIBS smart_monitor_core)
    endif()

    if(TARGET smart_monitor_agent)
        list(APPEND LINK_LIBS smart_monitor_agent)
    endif()

    target_link_libraries(smart_monitor_app ${LINK_LIBS})

    # Include directories
    target_include_directories(smart_monitor_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    )

    # Compiler flags
    target_compile_options(smart_monitor_app PRIVATE
        -Wall
        -Wextra
        -O2
        -g
    )

    # Installation
    install(TARGETS smart_monitor_app DESTINATION bin)
endif()

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
    set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
    
    add_custom_target(docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "Smart Monitor")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "IoT Baby Monitoring System")
set(CPACK_PACKAGE_VENDOR "Smart Monitor Project")
set(CPACK_GENERATOR "TGZ;DEB")

include(CPack)
