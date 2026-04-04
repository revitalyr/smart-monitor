/**
 * @file data_agent.h
 * @brief Data Agent for Smart Monitor system
 * 
 * This module provides centralized data collection, processing, and correlation
 * for the Smart Monitor system. It manages sensor data, audio processing,
 * video analysis, and maintains the baby state machine with realistic
 * physics-based simulation capabilities.
 */

#ifndef DATA_AGENT_H
#define DATA_AGENT_H

#include "../common/types.h"
#include "../common/smart_monitor_protocol.h"
#include "../ffi/rust_bridge.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque data agent structure
 */
typedef struct data_agent DataAgent;

/**
 * @brief Agent configuration structure
 * 
 * Contains configuration options for enabling/disabling various
 * components and setting thresholds and intervals.
 */
typedef struct {
    bool enable_camera;        /**< Enable camera/video processing */
    bool enable_audio;         /**< Enable audio processing */
    bool enable_sensors;       /**< Enable sensor reading */
    bool enable_simulation;     /**< Enable physics simulation */
    IntervalMs update_interval_ms; /**< Update interval in milliseconds */
    char* video_source;        /**< Video device path */
    MotionThreshold motion_threshold; /**< Motion detection threshold */
    MotionThreshold audio_threshold; /**< Audio detection threshold */
} AgentConfig;

/**
 * @brief Agent statistics structure
 * 
 * Contains runtime statistics for monitoring system performance
 * and debugging purposes.
 */
typedef struct {
    FrameCount frames_processed;  /**< Total video frames processed */
    FrameCount motion_events;     /**< Total motion events detected */
    FrameCount audio_events;      /**< Total audio events detected */
    FrameCount sensor_updates;    /**< Total sensor updates processed */
    TimestampMs start_time;        /**< Agent start timestamp */
    DurationMs uptime;           /**< Agent uptime in seconds */
} AgentStats;

/**
 * @brief Sensor data callback function type
 * 
 * @param data Pointer to sensor data structure
 * @param user_data User-provided data pointer
 */
typedef void (*SensorDataCallback)(const SensorData* data, void* user_data);

/**
 * @brief Audio data callback function type
 * 
 * @param data Pointer to audio data structure
 * @param user_data User-provided data pointer
 */
typedef void (*AudioDataCallback)(const AudioData* data, void* user_data);

/**
 * @brief Motion alert callback function type
 * 
 * @param alert Pointer to motion alert structure
 * @param user_data User-provided data pointer
 */
typedef void (*MotionAlertCallback)(const MotionAlert* alert, void* user_data);

/**
 * @brief System status callback function type
 * 
 * @param status Pointer to system status structure
 * @param user_data User-provided data pointer
 */
typedef void (*SystemStatusCallback)(const SystemStatus* status, void* user_data);

/**
 * @brief Create data agent instance
 * 
 * @param config Pointer to agent configuration structure
 * @return Pointer to DataAgent instance or NULL on failure
 */
DataAgent* data_agent_create(const AgentConfig* config);

/**
 * @brief Destroy data agent and free resources
 * 
 * @param agent Pointer to DataAgent instance
 */
void data_agent_destroy(DataAgent* agent);

/**
 * @brief Start data agent processing
 * 
 * @param agent Pointer to data_agent_t instance
 * @return true on success, false on failure
 */
bool data_agent_start(DataAgent* agent);

/**
 * @brief Stop data agent processing
 * 
 * @param agent Pointer to DataAgent instance
 */
void data_agent_stop(DataAgent* agent);

/**
 * @brief Check if data agent is running
 * 
 * @param agent Pointer to DataAgent instance
 * @return true if running, false otherwise
 */
bool data_agent_is_running(const DataAgent* agent);

/**
 * @brief Set sensor data callback
 * 
 * @param agent Pointer to DataAgent instance
 * @param callback Sensor data callback function
 * @param user_data User data to pass to callback
 */
void data_agent_set_sensor_callback(DataAgent* agent, SensorDataCallback callback, void* user_data);

/**
 * @brief Set audio data callback
 * 
 * @param agent Pointer to DataAgent instance
 * @param callback Audio data callback function
 * @param user_data User data to pass to callback
 */
void data_agent_set_audio_callback(DataAgent* agent, AudioDataCallback callback, void* user_data);

/**
 * @brief Set motion alert callback
 * 
 * @param agent Pointer to DataAgent instance
 * @param callback Motion alert callback function
 * @param user_data User data to pass to callback
 */
void data_agent_set_motion_callback(DataAgent* agent, MotionAlertCallback callback, void* user_data);

/**
 * @brief Set system status callback
 * 
 * @param agent Pointer to DataAgent instance
 * @param callback System status callback function
 * @param user_data User data to pass to callback
 */
void data_agent_set_status_callback(DataAgent* agent, SystemStatusCallback callback, void* user_data);

/**
 * @brief Get agent statistics
 * 
 * @param agent Pointer to DataAgent instance
 * @return Agent statistics structure
 */
AgentStats data_agent_get_stats(const DataAgent* agent);
/**
 * @brief Reset agent statistics
 * 
 * @param agent Pointer to DataAgent instance
 */
void data_agent_reset_stats(DataAgent* agent);

/**
 * @brief Set motion detection threshold
 * 
 * @param agent Pointer to DataAgent instance
 * @param threshold Motion detection threshold (0.0-1.0)
 */
void data_agent_set_motion_threshold(DataAgent* agent, float threshold);

/**
 * @brief Set audio detection threshold
 * 
 * @param agent Pointer to DataAgent instance
 * @param threshold Audio detection threshold (0.0-1.0)
 */
void data_agent_set_audio_threshold(DataAgent* agent, float threshold);

/**
 * @brief Set update interval
 * 
 * @param agent Pointer to DataAgent instance
 * @param interval_ms Update interval in milliseconds
 */
void data_agent_set_update_interval(DataAgent* agent, uint32_t interval_ms);

/**
 * @brief Set HTTP server JSON callbacks
 * 
 * @param agent Pointer to data_agent_t instance
 * @param sensor_cb Sensor JSON callback function
 * @param audio_cb Audio JSON callback function
 * @param system_cb System JSON callback function
 */
void data_agent_set_http_callbacks(DataAgent* agent, 
                                   char* (*sensor_cb)(void),
                                   char* (*audio_cb)(void),
                                   char* (*system_cb)(void));

/**
 * @brief Set video JSON callback
 * 
 * @param agent Pointer to DataAgent instance
 * @param video_cb Video JSON callback function
 */
void data_agent_set_video_callback(DataAgent* agent, char* (*video_cb)(void));

/**
 * @brief Get sensor data as JSON string
 * 
 * @param agent Pointer to DataAgent instance
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* data_agent_get_sensor_json(DataAgent* agent);

/**
 * @brief Get audio data as JSON string
 * 
 * @param agent Pointer to DataAgent instance
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* data_agent_get_audio_json(DataAgent* agent);

/**
 * @brief Get video data as JSON string
 * 
 * @param agent Pointer to DataAgent instance
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* data_agent_get_video_json(DataAgent* agent);

/**
 * @brief Get system data as JSON string
 * 
 * @param agent Pointer to DataAgent instance
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* data_agent_get_system_json(DataAgent* agent);

/**
 * @brief Generate sensor JSON data
 * 
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* generate_sensor_json(void);

/**
 * @brief Generate audio JSON data
 * 
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* generate_audio_json(void);

/**
 * @brief Generate video JSON data
 * 
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* generate_video_json(void);

/**
 * @brief Generate system status JSON data
 * 
 * @return Allocated JSON string (caller must free) or NULL on failure
 */
char* generate_system_json(void);

#ifdef __cplusplus
}
#endif

#endif // DATA_AGENT_H
