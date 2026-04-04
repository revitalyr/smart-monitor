cmake_minimum_required(VERSION 3.16)

# HTTP Server API
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/api/http_server.c)
    add_library(smart_monitor_http_api STATIC api/http_server.c)
    target_include_directories(smart_monitor_http_api PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_SOURCE_DIR}/src/core/common
    )
    target_compile_options(smart_monitor_http_api PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# Protocol Server
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/protocol/protocol_server.c)
    add_library(smart_monitor_protocol_server STATIC protocol/protocol_server.c)
    target_include_directories(smart_monitor_protocol_server PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol
        ${CMAKE_SOURCE_DIR}/src/core/common
    )
    target_compile_options(smart_monitor_protocol_server PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# WebRTC Server
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/webrtc/webrtc_server.c)
    add_library(smart_monitor_webrtc_server STATIC webrtc/webrtc_server.c)
    target_include_directories(smart_monitor_webrtc_server PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/webrtc
        ${CMAKE_SOURCE_DIR}/src/core/common
    )
    target_compile_options(smart_monitor_webrtc_server PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()
