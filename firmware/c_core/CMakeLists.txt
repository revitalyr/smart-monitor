cmake_minimum_required(VERSION 3.16)

# Project configuration
project(smart_monitor VERSION 1.0.0 LANGUAGES C)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/firmware/c_core)
include_directories(${CMAKE_SOURCE_DIR}/firmware/c_core/ffi)

# Build options
option(ENABLE_MOCK "Enable mock mode" OFF)
option(ENABLE_AUDIO "Enable audio processing" OFF)
option(ENABLE_SENSORS "Enable sensor support" OFF)
option(ENABLE_WEBRTC "Enable WebRTC streaming" OFF)
option(WITH_SYSTEMD "Enable systemd integration" OFF)
option(ENABLE_CURL "Enable HTTP client features" OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Core media libraries
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.16)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=1.16)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=1.16)

# Optional libraries
if(ENABLE_AUDIO)
    pkg_check_modules(ALSA REQUIRED alsa)
endif()

if(ENABLE_WEBRTC)
    find_package(PkgConfig QUIET)
    pkg_check_modules(WEBRTC QUIET libwebrtc)
    if(NOT WEBRTC_FOUND)
        message(WARNING "WebRTC library not found - WebRTC features will be disabled")
        set(ENABLE_WEBRTC OFF)
    endif()
endif()

if(ENABLE_CURL)
    find_package(CURL REQUIRED)
endif()

if(WITH_SYSTEMD)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD REQUIRED systemd)
endif()

# Add executable with conditional sources
set(SOURCES
    main.c
    video/v4l2_capture.c
    ffi/rust_bridge.c
    ${CMAKE_SOURCE_DIR}/web/http_server.c
    ${CMAKE_SOURCE_DIR}/web/protocol_server.c
    agent/data_agent.c
    common/smart_monitor_protocol.c
    audio/audio_capture.c
    audio/noise_detection.c
    sensors/i2c_sensor.c
    sensors/spi_device.c
    comm/uart_interface.c
    esp32/esp32_device.c
)

# Conditional source additions
if(ENABLE_AUDIO)
    list(APPEND SOURCES audio/audio_capture.c audio/noise_detection.c)
endif()

if(ENABLE_SENSORS)
    list(APPEND SOURCES 
        sensors/i2c_sensor.c 
        sensors/spi_device.c 
        comm/uart_interface.c 
        esp32/esp32_device.c)
endif()

if(ENABLE_WEBRTC)
    list(APPEND SOURCES net/webrtc_server.c)
endif()

add_executable(smart_monitor ${SOURCES})

# Link libraries
set(LINK_LIBS
    ${GST_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/firmware/rust_modules/motion/target/release/libmotion.a
    pthread
    m
    dl
)

# Conditional library additions
if(ENABLE_AUDIO)
    list(APPEND LINK_LIBS ${ALSA_LIBRARIES})
endif()

if(ENABLE_WEBRTC AND WEBRTC_FOUND)
    list(APPEND LINK_LIBS ${WEBRTC_LIBRARIES})
endif()

if(ENABLE_CURL)
    list(APPEND LINK_LIBS ${CURL_LIBRARIES})
endif()

if(WITH_SYSTEMD AND SYSTEMD_FOUND)
    list(APPEND LINK_LIBS ${SYSTEMD_LIBRARIES})
endif()

target_link_libraries(smart_monitor ${LINK_LIBS})

# Include directories
target_include_directories(smart_monitor PRIVATE
    ${GST_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
)

if(ENABLE_WEBRTC AND WEBRTC_FOUND)
    target_include_directories(smart_monitor PRIVATE ${WEBRTC_INCLUDE_DIRS})
    target_compile_options(smart_monitor PRIVATE ${WEBRTC_CFLAGS_OTHER})
endif()

if(ENABLE_AUDIO)
    target_include_directories(smart_monitor PRIVATE ${ALSA_INCLUDE_DIRS})
    target_compile_options(smart_monitor PRIVATE ${ALSA_CFLAGS_OTHER})
endif()

if(WITH_SYSTEMD AND SYSTEMD_FOUND)
    target_include_directories(smart_monitor PRIVATE ${SYSTEMD_INCLUDE_DIRS})
    target_compile_options(smart_monitor PRIVATE ${SYSTEMD_CFLAGS_OTHER})
endif()

# Compiler flags
target_compile_options(smart_monitor PRIVATE
    ${GST_CFLAGS_OTHER}
    ${GST_APP_CFLAGS_OTHER}
    ${GST_VIDEO_CFLAGS_OTHER}
    -Wall
    -Wextra
    -O2
    -D_GNU_SOURCE
)

# Conditional defines
if(ENABLE_MOCK)
    target_compile_definitions(smart_monitor PRIVATE ENABLE_MOCK)
endif()

if(ENABLE_AUDIO)
    target_compile_definitions(smart_monitor PRIVATE ENABLE_AUDIO)
endif()

if(ENABLE_SENSORS)
    target_compile_definitions(smart_monitor PRIVATE ENABLE_SENSORS)
endif()

# Installation
install(TARGETS smart_monitor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy HTML interface to build directory
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/web/static/dashboard.html
    ${CMAKE_BINARY_DIR}/firmware/c_core/static/dashboard.html
    COMMENT "Copying HTML dashboard to build directory"
)

# Copy static web assets to build directory
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/firmware/c_core/static
    COMMENT "Creating static directory"
)
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/web/static/main.css
    ${CMAKE_BINARY_DIR}/firmware/c_core/static/main.css
    COMMENT "Copying main.css to build directory"
)
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/firmware/c_core/scripts
    COMMENT "Creating scripts directory"
)
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/web/scripts/app.js
    ${CMAKE_BINARY_DIR}/firmware/c_core/scripts/app.js
    COMMENT "Copying app.js to build directory"
)
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/web/scripts/charts.js
    ${CMAKE_BINARY_DIR}/firmware/c_core/scripts/charts.js
    COMMENT "Copying charts.js to build directory"
)

# Generate signature for the binary
add_custom_command(TARGET smart_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E sha256sum smart_monitor > smart_monitor.sig
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/firmware/c_core
    COMMENT "Generating signature for smart_monitor binary"
)

# Install configuration file
if(EXISTS ${CMAKE_SOURCE_DIR}/config/smartmonitor.conf)
    install(FILES ${CMAKE_SOURCE_DIR}/config/smartmonitor.conf
        DESTINATION etc)
endif()

# Install systemd service
if(WITH_SYSTEMD AND EXISTS ${CMAKE_SOURCE_DIR}/systemd/smartmonitor.service)
    install(FILES ${CMAKE_SOURCE_DIR}/systemd/smartmonitor.service
        DESTINATION lib/systemd/user)
endif()
