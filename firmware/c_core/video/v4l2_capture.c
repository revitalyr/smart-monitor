#include "v4l2_capture.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static bool v4l2_configure_format(v4l2_capture_t* cap);
static bool v4l2_init_mmap_buffers(v4l2_capture_t* cap);

v4l2_capture_t* v4l2_create(const char* device) {
    if (!device) {
        return NULL;
    }
    
    v4l2_capture_t* cap = malloc(sizeof(v4l2_capture_t));
    if (!cap) {
        return NULL;
    }
    
    memset(cap, 0, sizeof(v4l2_capture_t));
    strncpy(cap->device, device, sizeof(cap->device) - 1);
    cap->fd = -1;
    cap->width = 640;
    cap->height = 480;
    
    return cap;
}

void v4l2_destroy(v4l2_capture_t* cap) {
    if (!cap) {
        return;
    }
    
    v4l2_stop_capture(cap);
    
    if (cap->buffers) {
        for (size_t i = 0; i < cap->buffer_count; ++i) {
            if (cap->buffers[i].start != MAP_FAILED) {
                munmap(cap->buffers[i].start, cap->buffers[i].length);
            }
        }
        free(cap->buffers);
    }
    
    if (cap->fd >= 0) {
        close(cap->fd);
    }
    
    free(cap);
}

bool v4l2_initialize(v4l2_capture_t* cap, int width, int height) {
    if (!cap) {
        return false;
    }
    
    cap->width = width;
    cap->height = height;
    
    cap->fd = open(cap->device, O_RDWR | O_NONBLOCK);
    if (cap->fd < 0) {
        fprintf(stderr, "Cannot open device: %s (%s)\n", cap->device, strerror(errno));
        return false;
    }
    
    if (!v4l2_configure_format(cap)) {
        close(cap->fd);
        cap->fd = -1;
        return false;
    }
    
    if (!v4l2_init_mmap_buffers(cap)) {
        close(cap->fd);
        cap->fd = -1;
        return false;
    }
    
    cap->initialized = true;
    return true;
}

static bool v4l2_configure_format(v4l2_capture_t* cap) {
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = cap->width;
    fmt.fmt.pix.height = cap->height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    
    if (ioctl(cap->fd, VIDIOC_S_FMT, &fmt) < 0) {
        fprintf(stderr, "Failed to set format: %s\n", strerror(errno));
        return false;
    }
    
    cap->width = fmt.fmt.pix.width;
    cap->height = fmt.fmt.pix.height;
    cap->frame_size = fmt.fmt.pix.sizeimage;
    
    printf("Format set: %dx%d, frame size: %zu\n", cap->width, cap->height, cap->frame_size);
    return true;
}

static bool v4l2_init_mmap_buffers(v4l2_capture_t* cap) {
    struct v4l2_requestbuffers req = {};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(cap->fd, VIDIOC_REQBUFS, &req) < 0) {
        fprintf(stderr, "Failed to request buffers: %s\n", strerror(errno));
        return false;
    }
    
    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        return false;
    }
    
    cap->buffers = malloc(req.count * sizeof(v4l2_buffer_t));
    if (!cap->buffers) {
        fprintf(stderr, "Failed to allocate buffer array\n");
        return false;
    }
    
    cap->buffer_count = req.count;
    memset(cap->buffers, 0, req.count * sizeof(v4l2_buffer_t));
    
    for (size_t i = 0; i < cap->buffer_count; ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(cap->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            fprintf(stderr, "Failed to query buffer: %s\n", strerror(errno));
            return false;
        }
        
        cap->buffers[i].length = buf.length;
        cap->buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, 
                                   MAP_SHARED, cap->fd, buf.m.offset);
        
        if (cap->buffers[i].start == MAP_FAILED) {
            fprintf(stderr, "Failed to mmap buffer: %s\n", strerror(errno));
            return false;
        }
    }
    
    return true;
}

bool v4l2_start_capture(v4l2_capture_t* cap) {
    if (!cap || !cap->initialized) {
        return false;
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    for (size_t i = 0; i < cap->buffer_count; ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(cap->fd, VIDIOC_QBUF, &buf) < 0) {
            fprintf(stderr, "Failed to queue buffer: %s\n", strerror(errno));
            return false;
        }
    }
    
    if (ioctl(cap->fd, VIDIOC_STREAMON, &type) < 0) {
        fprintf(stderr, "Failed to start streaming: %s\n", strerror(errno));
        return false;
    }
    
    return true;
}

void v4l2_stop_capture(v4l2_capture_t* cap) {
    if (!cap || cap->fd < 0) {
        return;
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cap->fd, VIDIOC_STREAMOFF, &type);
}

uint8_t* v4l2_read_frame(v4l2_capture_t* cap, size_t* frame_size) {
    if (!cap || !frame_size) {
        return NULL;
    }
    
    *frame_size = 0;
    
    if (cap->fd < 0) {
        return NULL;
    }
    
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(cap->fd, VIDIOC_DQBUF, &buf) < 0) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Failed to dequeue buffer: %s\n", strerror(errno));
        }
        return NULL;
    }
    
    uint8_t* frame_data = malloc(buf.bytesused);
    if (!frame_data) {
        fprintf(stderr, "Failed to allocate frame memory\n");
        return NULL;
    }
    
    memcpy(frame_data, cap->buffers[buf.index].start, buf.bytesused);
    *frame_size = buf.bytesused;
    
    if (ioctl(cap->fd, VIDIOC_QBUF, &buf) < 0) {
        fprintf(stderr, "Failed to requeue buffer: %s\n", strerror(errno));
    }
    
    return frame_data;
}

uint8_t* v4l2_read_frame_raw(v4l2_capture_t* cap, size_t* frame_size) {
    uint8_t* frame_data = v4l2_read_frame(cap, frame_size);
    
    if (!frame_data) {
        // Return mock frame for testing
        size_t mock_size = cap->width * cap->height * 2;
        uint8_t* mock_frame = malloc(mock_size);
        if (mock_frame) {
            memset(mock_frame, 128, mock_size);
            *frame_size = mock_size;
        }
        return mock_frame;
    }
    
    return frame_data;
}

bool v4l2_is_initialized(const v4l2_capture_t* cap) {
    return cap && cap->initialized;
}

int v4l2_get_width(const v4l2_capture_t* cap) {
    return cap ? cap->width : 0;
}

int v4l2_get_height(const v4l2_capture_t* cap) {
    return cap ? cap->height : 0;
}

size_t v4l2_get_frame_size(const v4l2_capture_t* cap) {
    return cap ? cap->frame_size : 0;
}
