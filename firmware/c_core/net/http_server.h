#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../common/smart_monitor_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    motion_events_count_t m_motion_events;
    motion_level_t m_motion_level;
    frame_count_t m_frames_processed;
    bool m_camera_active;
    char m_last_motion_time[32];
    char m_uptime[32];
} metrics_data_t;

typedef struct http_server http_server_t;

typedef struct {
    char* data;
    size_t size;
} http_response_t;

// Callback function types
typedef char* (*metrics_callback_t)(void);
typedef char* (*health_callback_t)(void);
typedef char* (*webrtc_callback_t)(void);

// Functions
http_server_t* http_server_create(port_t port);
void http_server_destroy(http_server_t* server);

bool http_server_start(http_server_t* server);
void http_server_stop(http_server_t* server);
bool http_server_is_running(const http_server_t* server);

void http_server_set_metrics_callback(http_server_t* server, metrics_callback_t callback);
void http_server_set_health_callback(http_server_t* server, health_callback_t callback);
void http_server_set_webrtc_callback(http_server_t* server, webrtc_callback_t callback);

metrics_data_t* http_server_get_metrics(http_server_t* server);

// Utility functions
char* generate_metrics_json(const metrics_data_t* metrics);
char* generate_health_json(void);
char* generate_webrtc_sdp(void);

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H
