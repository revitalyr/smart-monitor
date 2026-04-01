cmake_minimum_required(VERSION 3.16)
project(smart_monitor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.16)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=1.16)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=1.16)

# Add subdirectories
add_subdirectory(firmware/c_core)

# Rust static library build
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/target/release/libmotion.a
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/firmware/rust_modules/motion
)

add_custom_target(rustlib ALL
    DEPENDS ${CMAKE_SOURCE_DIR}/target/release/libmotion.a
)

# Custom target to build everything
add_custom_target(build_all
    DEPENDS smart_monitor rustlib
)
