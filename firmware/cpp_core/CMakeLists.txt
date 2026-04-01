cmake_minimum_required(VERSION 3.16)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/firmware/cpp_core)
include_directories(${CMAKE_SOURCE_DIR}/firmware/cpp_core/ffi)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.16)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=1.16)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=1.16)

# Find Google Test
find_package(GTest REQUIRED)

# Add executable
add_executable(smart_monitor
    main.cpp
    video/v4l2_capture.cpp
    video/gstreamer_pipeline.cpp
    ffi/rust_bridge.cpp
    net/http_server.cpp
    net/webrtc_server.cpp
)

# Link libraries
target_link_libraries(smart_monitor
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/firmware/rust_modules/motion/target/release/libmotion.a
    pthread
    dl
)

# Include directories for GStreamer
target_include_directories(smart_monitor PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
)

# Compiler flags
target_compile_options(smart_monitor PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${GST_VIDEO_CFLAGS_OTHER}
)

# Enable testing
enable_testing()

# Test executable
add_executable(smart_monitor_tests
    tests/test_v4l2_capture.cpp
    tests/test_rust_bridge.cpp
    tests/test_http_server.cpp
    tests/test_integration.cpp
    video/v4l2_capture.cpp
    video/gstreamer_pipeline.cpp
    ffi/rust_bridge.cpp
    net/http_server.cpp
    net/webrtc_server.cpp
)

# Link test libraries
target_link_libraries(smart_monitor_tests
    GTest::gtest
    GTest::gtest_main
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/firmware/rust_modules/motion/target/release/libmotion.a
    pthread
    dl
    curl
)

# Include directories for tests
target_include_directories(smart_monitor_tests PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
)

# Compiler flags for tests
target_compile_options(smart_monitor_tests PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${GST_VIDEO_CFLAGS_OTHER}
)

# Add tests
add_test(NAME AllTests COMMAND smart_monitor_tests)

# Individual test suites
add_test(NAME V4L2Tests COMMAND smart_monitor_tests --gtest_filter="V4L2CaptureTest.*")
add_test(NAME RustBridgeTests COMMAND smart_monitor_tests --gtest_filter="RustBridgeTest.*")
add_test(NAME HTTPServerTests COMMAND smart_monitor_tests --gtest_filter="HTTPServerTest.*")
add_test(NAME IntegrationTests COMMAND smart_monitor_tests --gtest_filter="IntegrationTest.*")
