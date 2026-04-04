cmake_minimum_required(VERSION 3.16)

# Rust FFI bridge library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ffi/rust_bridge.c)
    add_library(smart_monitor_ffi STATIC ffi/rust_bridge.c)
    target_include_directories(smart_monitor_ffi PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ffi
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_ffi PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()
