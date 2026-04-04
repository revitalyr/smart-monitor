cmake_minimum_required(VERSION 3.16)

# WebRTC Server
add_library(smart_monitor_webrtc_server STATIC webrtc_server.c)

target_include_directories(smart_monitor_webrtc_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core/common
)

target_compile_options(smart_monitor_webrtc_server PRIVATE
    -Wall
    -Wextra
    -g
)
