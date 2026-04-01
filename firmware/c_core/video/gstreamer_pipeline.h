#ifndef GSTREAMER_PIPELINE_H
#define GSTREAMER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gst_pipeline gst_pipeline_t;
typedef struct gst_webrtc_pipeline gst_webrtc_pipeline_t;

typedef void (*frame_callback_t)(const uint8_t* frame_data, size_t frame_size, void* user_data);

// Regular GStreamer Pipeline
gst_pipeline_t* gst_pipeline_create(const char* pipeline_str);
void gst_pipeline_destroy(gst_pipeline_t* pipeline);

bool gst_pipeline_start(gst_pipeline_t* pipeline);
void gst_pipeline_stop(gst_pipeline_t* pipeline);

uint8_t* gst_pipeline_get_frame(gst_pipeline_t* pipeline, size_t* frame_size);
bool gst_pipeline_has_frame(gst_pipeline_t* pipeline);

bool gst_pipeline_is_initialized(const gst_pipeline_t* pipeline);
bool gst_pipeline_is_playing(const gst_pipeline_t* pipeline);

void gst_pipeline_set_frame_callback(gst_pipeline_t* pipeline, frame_callback_t callback, void* user_data);

// WebRTC Pipeline
gst_webrtc_pipeline_t* gst_webrtc_pipeline_create(const char* device);
void gst_webrtc_pipeline_destroy(gst_webrtc_pipeline_t* pipeline);

bool gst_webrtc_pipeline_initialize(gst_webrtc_pipeline_t* pipeline, const char* device);
bool gst_webrtc_pipeline_start(gst_webrtc_pipeline_t* pipeline);
void gst_webrtc_pipeline_stop(gst_webrtc_pipeline_t* pipeline);

char* gst_webrtc_pipeline_generate_offer(gst_webrtc_pipeline_t* pipeline);
bool gst_webrtc_pipeline_set_answer(gst_webrtc_pipeline_t* pipeline, const char* answer);

bool gst_webrtc_pipeline_is_initialized(const gst_webrtc_pipeline_t* pipeline);
bool gst_webrtc_pipeline_is_running(const gst_webrtc_pipeline_t* pipeline);

#ifdef __cplusplus
}
#endif

#endif // GSTREAMER_PIPELINE_H
