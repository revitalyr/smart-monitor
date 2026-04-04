/**
 * @file gstreamer_pipeline.h
 * @brief GStreamer pipeline interface for Smart Monitor video processing
 * 
 * This module provides GStreamer-based video capture and processing functionality
 * including regular pipelines and WebRTC-based real-time video streaming.
 * Supports various video sources and formats with callback-based frame processing.
 */

#ifndef GSTREAMER_PIPELINE_H
#define GSTREAMER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque GStreamer pipeline structure
 */
typedef struct gst_pipeline gst_pipeline_t;

/**
 * @brief Opaque GStreamer WebRTC pipeline structure
 */
typedef struct gst_webrtc_pipeline gst_webrtc_pipeline_t;

/**
 * @brief Frame callback function type
 * 
 * @param frame_data Pointer to frame data buffer
 * @param frame_size Size of frame data in bytes
 * @param user_data User-provided data pointer
 */
typedef void (*frame_callback_t)(const uint8_t* frame_data, size_t frame_size, void* user_data);

/**
 * @brief Create GStreamer pipeline from description string
 * 
 * @param pipeline_str GStreamer pipeline description string
 * @return Pointer to gst_pipeline_t instance or NULL on failure
 */
gst_pipeline_t* gst_pipeline_create(const char* pipeline_str);

/**
 * @brief Destroy GStreamer pipeline and free resources
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 */
void gst_pipeline_destroy(gst_pipeline_t* pipeline);

/**
 * @brief Start GStreamer pipeline
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @return true on success, false on failure
 */
bool gst_pipeline_start(gst_pipeline_t* pipeline);

/**
 * @brief Stop GStreamer pipeline
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 */
void gst_pipeline_stop(gst_pipeline_t* pipeline);

/**
 * @brief Get current frame from pipeline
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @param frame_size Pointer to store frame size
 * @return Pointer to frame data or NULL if no frame available
 */
uint8_t* gst_pipeline_get_frame(gst_pipeline_t* pipeline, size_t* frame_size);

/**
 * @brief Check if pipeline has available frame
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @return true if frame is available, false otherwise
 */
bool gst_pipeline_has_frame(gst_pipeline_t* pipeline);

/**
 * @brief Check if pipeline is initialized
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @return true if initialized, false otherwise
 */
bool gst_pipeline_is_initialized(const gst_pipeline_t* pipeline);

/**
 * @brief Check if pipeline is playing
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @return true if playing, false otherwise
 */
bool gst_pipeline_is_playing(const gst_pipeline_t* pipeline);

/**
 * @brief Set frame callback for pipeline
 * 
 * @param pipeline Pointer to gst_pipeline_t instance
 * @param callback Frame callback function
 * @param user_data User data to pass to callback
 */
void gst_pipeline_set_frame_callback(gst_pipeline_t* pipeline, frame_callback_t callback, void* user_data);

/**
 * @brief Create WebRTC pipeline for real-time video streaming
 * 
 * @param device Video device path (e.g., "/dev/video0")
 * @return Pointer to gst_webrtc_pipeline_t instance or NULL on failure
 */
gst_webrtc_pipeline_t* gst_webrtc_pipeline_create(const char* device);

/**
 * @brief Destroy WebRTC pipeline and free resources
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 */
void gst_webrtc_pipeline_destroy(gst_webrtc_pipeline_t* pipeline);

/**
 * @brief Initialize WebRTC pipeline with device
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @param device Video device path
 * @return true on success, false on failure
 */
bool gst_webrtc_pipeline_initialize(gst_webrtc_pipeline_t* pipeline, const char* device);

/**
 * @brief Start WebRTC pipeline
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @return true on success, false on failure
 */
bool gst_webrtc_pipeline_start(gst_webrtc_pipeline_t* pipeline);

/**
 * @brief Stop WebRTC pipeline
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 */
void gst_webrtc_pipeline_stop(gst_webrtc_pipeline_t* pipeline);

/**
 * @brief Generate WebRTC offer for peer connection
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @return SDP offer string or NULL on failure
 */
char* gst_webrtc_pipeline_generate_offer(gst_webrtc_pipeline_t* pipeline);

/**
 * @brief Set WebRTC answer from remote peer
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @param answer SDP answer string from remote peer
 * @return true on success, false on failure
 */
bool gst_webrtc_pipeline_set_answer(gst_webrtc_pipeline_t* pipeline, const char* answer);

/**
 * @brief Check if WebRTC pipeline is initialized
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @return true if initialized, false otherwise
 */
bool gst_webrtc_pipeline_is_initialized(const gst_webrtc_pipeline_t* pipeline);

/**
 * @brief Check if WebRTC pipeline is running
 * 
 * @param pipeline Pointer to gst_webrtc_pipeline_t instance
 * @return true if running, false otherwise
 */
bool gst_webrtc_pipeline_is_running(const gst_webrtc_pipeline_t* pipeline);

#ifdef __cplusplus
}
#endif

#endif // GSTREAMER_PIPELINE_H
