#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* start;
    size_t length;
} v4l2_buffer_t;

typedef struct {
    int fd;
    char device[256];
    int width;
    int height;
    size_t frame_size;
    v4l2_buffer_t* buffers;
    size_t buffer_count;
    bool initialized;
} v4l2_capture_t;

// Functions
v4l2_capture_t* v4l2_create(const char* device);
void v4l2_destroy(v4l2_capture_t* cap);

bool v4l2_initialize(v4l2_capture_t* cap, int width, int height);
bool v4l2_start_capture(v4l2_capture_t* cap);
void v4l2_stop_capture(v4l2_capture_t* cap);

uint8_t* v4l2_read_frame(v4l2_capture_t* cap, size_t* frame_size);
uint8_t* v4l2_read_frame_raw(v4l2_capture_t* cap, size_t* frame_size);

bool v4l2_is_initialized(const v4l2_capture_t* cap);
int v4l2_get_width(const v4l2_capture_t* cap);
int v4l2_get_height(const v4l2_capture_t* cap);
size_t v4l2_get_frame_size(const v4l2_capture_t* cap);

#ifdef __cplusplus
}
#endif

#endif // V4L2_CAPTURE_H
