#include "v4l2_capture.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cerrno>
#include <cstring>
#include <iostream>

V4L2Capture::V4L2Capture(const std::string& device) 
    : fd_(-1), device_(device), width_(640), height_(480), frame_size_(0) {
}

V4L2Capture::~V4L2Capture() {
    stopCapture();
}

bool V4L2Capture::initialize(int width, int height) {
    width_ = width;
    height_ = height;
    
    fd_ = open(device_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ < 0) {
        std::cerr << "Cannot open device: " << device_ << " (" << strerror(errno) << ")" << std::endl;
        return false;
    }
    
    if (!configureFormat()) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    
    if (!initMmapBuffers()) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    
    return true;
}

bool V4L2Capture::configureFormat() {
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width_;
    fmt.fmt.pix.height = height_;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    
    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "Failed to set format: " << strerror(errno) << std::endl;
        return false;
    }
    
    width_ = fmt.fmt.pix.width;
    height_ = fmt.fmt.pix.height;
    frame_size_ = fmt.fmt.pix.sizeimage;
    
    std::cout << "Format set: " << width_ << "x" << height_ 
              << ", frame size: " << frame_size_ << std::endl;
    
    return true;
}

bool V4L2Capture::initMmapBuffers() {
    struct v4l2_requestbuffers req = {};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(fd_, VIDIOC_REQBUFS, &req) < 0) {
        std::cerr << "Failed to request buffers: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (req.count < 2) {
        std::cerr << "Insufficient buffer memory" << std::endl;
        return false;
    }
    
    buffers_.resize(req.count);
    
    for (size_t i = 0; i < buffers_.size(); ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(fd_, VIDIOC_QUERYBUF, &buf) < 0) {
            std::cerr << "Failed to query buffer: " << strerror(errno) << std::endl;
            return false;
        }
        
        buffers_[i].length = buf.length;
        buffers_[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, 
                               MAP_SHARED, fd_, buf.m.offset);
        
        if (buffers_[i].start == MAP_FAILED) {
            std::cerr << "Failed to mmap buffer: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    return true;
}

void V4L2Capture::uninitMmapBuffers() {
    for (auto& buffer : buffers_) {
        if (buffer.start != MAP_FAILED) {
            munmap(buffer.start, buffer.length);
        }
    }
    buffers_.clear();
}

bool V4L2Capture::startCapture() {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    for (size_t i = 0; i < buffers_.size(); ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        
        if (ioctl(fd_, VIDIOC_QBUF, &buf) < 0) {
            std::cerr << "Failed to queue buffer: " << strerror(errno) << std::endl;
            return false;
        }
    }
    
    if (ioctl(fd_, VIDIOC_STREAMON, &type) < 0) {
        std::cerr << "Failed to start streaming: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

void V4L2Capture::stopCapture() {
    if (fd_ >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd_, VIDIOC_STREAMOFF, &type);
        
        uninitMmapBuffers();
        close(fd_);
        fd_ = -1;
    }
}

std::vector<uint8_t> V4L2Capture::readFrame() {
    if (fd_ < 0) {
        return std::vector<uint8_t>();
    }
    
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
    if (ioctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
        if (errno != EAGAIN) {
            std::cerr << "Failed to dequeue buffer: " << strerror(errno) << std::endl;
        }
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> frame_data;
    frame_data.reserve(buf.bytesused);
    frame_data.assign(
        static_cast<uint8_t*>(buffers_[buf.index].start),
        static_cast<uint8_t*>(buffers_[buf.index].start) + buf.bytesused
    );
    
    if (ioctl(fd_, VIDIOC_QBUF, &buf) < 0) {
        std::cerr << "Failed to requeue buffer: " << strerror(errno) << std::endl;
    }
    
    return frame_data;
}

std::vector<uint8_t> V4L2Capture::readFrameRaw() {
    std::vector<uint8_t> frame_data = readFrame();
    
    if (frame_data.empty()) {
        return std::vector<uint8_t>(width_ * height_ * 2, 128);
    }
    
    return frame_data;
}
