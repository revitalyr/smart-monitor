#ifndef DATA_AGENT_H
#define DATA_AGENT_H

#include "../common/smart_monitor_protocol.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct data_agent data_agent_t;

// Agent configuration
typedef struct {
    bool enable_camera;
    bool enable_audio;
    bool enable_sensors;
    bool enable_simulation;
    uint32_t update_interval_ms;
    char* video_source;
    float motion_threshold;
    float audio_threshold;
} agent_config_t;

// Agent statistics
typedef struct {
    uint32_t frames_processed;
    uint32_t motion_events;
    uint32_t audio_events;
    uint32_t sensor_updates;
    uint64_t start_time;
    uint32_t uptime;
} agent_stats_t;

// Callback function types
typedef void (*sensor_data_callback_t)(const sensor_data_t* data, void* user_data);
typedef void (*audio_data_callback_t)(const audio_data_t* data, void* user_data);
typedef void (*motion_alert_callback_t)(const motion_alert_t* alert, void* user_data);
typedef void (*system_status_callback_t)(const system_status_t* status, void* user_data);

// Agent lifecycle functions
data_agent_t* data_agent_create(const agent_config_t* config);
void data_agent_destroy(data_agent_t* agent);

// Agent control functions
bool data_agent_start(data_agent_t* agent);
void data_agent_stop(data_agent_t* agent);
bool data_agent_is_running(const data_agent_t* agent);

// Callback registration functions
void data_agent_set_sensor_callback(data_agent_t* agent, sensor_data_callback_t callback, void* user_data);
void data_agent_set_audio_callback(data_agent_t* agent, audio_data_callback_t callback, void* user_data);
void data_agent_set_motion_callback(data_agent_t* agent, motion_alert_callback_t callback, void* user_data);
void data_agent_set_status_callback(data_agent_t* agent, system_status_callback_t callback, void* user_data);

// Agent statistics
agent_stats_t data_agent_get_stats(const data_agent_t* agent);
void data_agent_reset_stats(data_agent_t* agent);

// Agent configuration
void data_agent_set_motion_threshold(data_agent_t* agent, float threshold);
void data_agent_set_audio_threshold(data_agent_t* agent, float threshold);
void data_agent_set_update_interval(data_agent_t* agent, uint32_t interval_ms);

// HTTP server JSON callbacks
void data_agent_set_http_callbacks(data_agent_t* agent, 
                                   char* (*sensor_cb)(void),
                                   char* (*audio_cb)(void),
                                   char* (*system_cb)(void));

// Get JSON data functions
char* data_agent_get_sensor_json(data_agent_t* agent);
char* data_agent_get_audio_json(data_agent_t* agent);
char* data_agent_get_system_json(data_agent_t* agent);

// JSON generator functions (extern for main.c)
char* generate_sensor_json(void);
char* generate_audio_json(void);
char* generate_system_json(void);

#ifdef __cplusplus
}
#endif

#endif // DATA_AGENT_H
