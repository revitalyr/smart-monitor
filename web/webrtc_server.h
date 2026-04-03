#ifndef WEBRTC_SERVER_H
#define WEBRTC_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct webrtc_server webrtc_server_t;

// Functions
webrtc_server_t* webrtc_server_create(const char* device);
void webrtc_server_destroy(webrtc_server_t* server);

bool webrtc_server_initialize(webrtc_server_t* server, const char* device);
bool webrtc_server_start(webrtc_server_t* server);
void webrtc_server_stop(webrtc_server_t* server);

char* webrtc_server_generate_offer(webrtc_server_t* server);
bool webrtc_server_set_answer(webrtc_server_t* server, const char* answer);

bool webrtc_server_is_initialized(const webrtc_server_t* server);
bool webrtc_server_is_running(const webrtc_server_t* server);

#ifdef __cplusplus
}
#endif

#endif // WEBRTC_SERVER_H
