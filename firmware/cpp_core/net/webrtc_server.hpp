#pragma once
#include "video/gstreamer_pipeline.hpp"
#include <string>
#include <memory>

class WebRTCServer {
public:
    WebRTCServer();
    ~WebRTCServer();
    
    bool initialize(const std::string& device = "/dev/video0");
    bool start();
    void stop();
    
    std::string generateOffer();
    bool setAnswer(const std::string& answer);
    
    bool isInitialized() const { return webrtc_pipeline_ && webrtc_pipeline_->isInitialized(); }
    bool isRunning() const { return webrtc_pipeline_ && webrtc_pipeline_->isPlaying(); }

private:
    std::unique_ptr<GstWebRTCPipeline> webrtc_pipeline_;
    std::string current_device_;
};
