#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct V4L2Buffer {
    void* start;
    size_t length;
};

class V4L2Capture {
public:
    explicit V4L2Capture(const std::string& device = "/dev/video0");
    ~V4L2Capture();
    
    bool initialize(int width = 640, int height = 480);
    bool startCapture();
    void stopCapture();
    
    std::vector<uint8_t> readFrame();
    std::vector<uint8_t> readFrameRaw();
    
    bool isInitialized() const { return fd_ >= 0; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    size_t getFrameSize() const { return frame_size_; }

private:
    int fd_;
    std::string device_;
    int width_;
    int height_;
    size_t frame_size_;
    std::vector<V4L2Buffer> buffers_;
    
    bool initMmapBuffers();
    void uninitMmapBuffers();
    bool configureFormat();
};
