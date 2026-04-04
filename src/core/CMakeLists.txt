cmake_minimum_required(VERSION 3.16)

# Core library sources
set(CORE_SOURCES)

# Add sources based on what exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/agent/data_agent.c)
    list(APPEND CORE_SOURCES agent/data_agent.c)
    message(STATUS "Found data_agent.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_capture.c)
    list(APPEND CORE_SOURCES audio/audio_capture.c)
    message(STATUS "Found audio_capture.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sensors/i2c_sensor.c)
    list(APPEND CORE_SOURCES sensors/i2c_sensor.c)
    message(STATUS "Found i2c_sensor.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/smart_monitor_protocol.c)
    list(APPEND CORE_SOURCES common/smart_monitor_protocol.c)
    message(STATUS "Found smart_monitor_protocol.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/globals.c)
    list(APPEND CORE_SOURCES common/globals.c)
    message(STATUS "Found globals.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/video/v4l2_capture.c)
    list(APPEND CORE_SOURCES video/v4l2_capture.c)
    message(STATUS "Found v4l2_capture.c")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ffi/rust_bridge.c)
    list(APPEND CORE_SOURCES ffi/rust_bridge_mock.c)
    message(STATUS "Found rust_bridge_mock.c")
endif()

# Create individual module libraries
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/agent/data_agent.c)
    add_library(smart_monitor_agent STATIC agent/data_agent.c)
    target_include_directories(smart_monitor_agent PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_agent PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/audio/audio_capture.c)
    add_library(smart_monitor_audio STATIC audio/audio_capture.c)
    target_include_directories(smart_monitor_audio PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_audio PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sensors/i2c_sensor.c)
    add_library(smart_monitor_sensors STATIC sensors/i2c_sensor.c)
    target_include_directories(smart_monitor_sensors PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_sensors PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# Common library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/smart_monitor_protocol.c)
    add_library(smart_monitor_common STATIC common/smart_monitor_protocol.c)
    target_include_directories(smart_monitor_common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_common PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# Communication library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/communication/uart_interface.c)
    add_library(smart_monitor_communication STATIC communication/uart_interface.c)
    target_include_directories(smart_monitor_communication PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/communication
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )
    target_compile_options(smart_monitor_communication PRIVATE
        -Wall
        -Wextra
        -g
    )
endif()

# Combined core library
if(CORE_SOURCES)
    add_library(smart_monitor_core STATIC ${CORE_SOURCES})

    target_include_directories(smart_monitor_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
    )

    target_link_libraries(smart_monitor_core
        ${CMAKE_THREAD_LIBS_INIT}
        m
    )

    target_compile_options(smart_monitor_core PRIVATE
        -Wall
        -Wextra
        -g
        -O2
    )

    # Set properties
    set_target_properties(smart_monitor_core PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
endif()
