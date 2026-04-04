cmake_minimum_required(VERSION 3.16)

# HTTP Server API
add_library(smart_monitor_http_api STATIC http_server.c)

target_include_directories(smart_monitor_http_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core/common
)

target_compile_options(smart_monitor_http_api PRIVATE
    -Wall
    -Wextra
    -g
)
