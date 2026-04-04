cmake_minimum_required(VERSION 3.16)

# Video capture library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/video/v4l2_capture.c)
    add_library(smart_monitor_video STATIC video/v4l2_capture.c)
    target_include_directories(smart_monitor_video PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/video
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_video PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# GStreamer pipeline library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/video/gstreamer_pipeline.c)
    add_library(smart_monitor_gstreamer STATIC video/gstreamer_pipeline.c)
    target_include_directories(smart_monitor_gstreamer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/video
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_gstreamer PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()
