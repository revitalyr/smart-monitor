#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include "../common/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* start;
    ByteCount length;
} V4L2Buffer;

typedef struct {
    FileDescriptor fd;
    char device[256];
    FrameWidth width;
    FrameHeight height;
    ByteCount frame_size;
    V4L2Buffer* buffers;
    size_t buffer_count;
    bool initialized;
    bool mock_mode;
} V4L2Capture;

// Functions
V4L2Capture* v4l2_create(const char* device);
void v4l2_destroy(V4L2Capture* cap);

bool v4l2_initialize(V4L2Capture* cap, FrameWidth width, FrameHeight height);
bool v4l2_start_capture(V4L2Capture* cap);
void v4l2_stop_capture(V4L2Capture* cap);

uint8_t* v4l2_read_frame(V4L2Capture* cap, ByteCount* frame_size);
uint8_t* v4l2_read_frame_raw(V4L2Capture* cap, ByteCount* frame_size);

bool v4l2_is_initialized(const V4L2Capture* cap);
FrameWidth v4l2_get_width(const V4L2Capture* cap);
FrameHeight v4l2_get_height(const V4L2Capture* cap);
ByteCount v4l2_get_frame_size(const V4L2Capture* cap);

#ifdef __cplusplus
}
#endif

#endif // V4L2_CAPTURE_H
