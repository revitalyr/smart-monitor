#pragma once
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

class GstPipeline {
public:
    GstPipeline();
    ~GstPipeline();
    
    bool initialize(const std::string& pipeline_str);
    bool start();
    void stop();
    
    std::vector<uint8_t> getFrame();
    bool hasFrame();
    
    bool isInitialized() const { return pipeline_ != nullptr; }
    bool isPlaying() const;
    
    void setNewFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback);

private:
    GstElement* pipeline_;
    GstElement* appsink_;
    std::function<void(const std::vector<uint8_t>&)> frame_callback_;
    
    static GstFlowReturn onNewSample(GstAppSink* appsink, gpointer user_data);
    std::vector<uint8_t> processSample(GstSample* sample);
};

class GstWebRTCPipeline {
public:
    GstWebRTCPipeline();
    ~GstWebRTCPipeline();
    
    bool initialize(const std::string& device = "/dev/video0");
    bool start();
    void stop();
    
    std::string generateOffer();
    bool setAnswer(const std::string& answer);
    
    bool isInitialized() const { return pipeline_ != nullptr; }

private:
    GstElement* pipeline_;
    GstElement* webrtcbin_;
    
    static void onOfferCreated(GstPromise* promise, gpointer user_data);
    static void onIceCandidate(GstElement* webrtc, guint mlineindex, gchar* cand, gpointer user_data);
};
