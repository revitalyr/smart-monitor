cmake_minimum_required(VERSION 3.16)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/firmware/c_core)
include_directories(${CMAKE_SOURCE_DIR}/firmware/c_core/ffi)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.16)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=1.16)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=1.16)

# Add executable
add_executable(smart_monitor
    main.c
    video/v4l2_capture.c
    video/gstreamer_pipeline.c
    ffi/rust_bridge.c
    net/http_server.c
    net/webrtc_server.c
)

# Link libraries
target_link_libraries(smart_monitor
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/firmware/rust_modules/motion/target/release/libmotion.a
    pthread
    m
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
    -Wall
    -Wextra
    -O2
)
