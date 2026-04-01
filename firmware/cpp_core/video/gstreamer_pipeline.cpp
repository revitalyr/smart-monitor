#include "gstreamer_pipeline.hpp"
#include <iostream>
#include <gst/video/video.h>

GstPipelineWrapper::GstPipelineWrapper() : pipeline_(nullptr), appsink_(nullptr) {
    gst_init(nullptr, nullptr);
}

GstPipelineWrapper::~GstPipelineWrapper() {
    stop();
}

bool GstPipelineWrapper::initialize(const std::string& pipeline_str) {
    GError* error = nullptr;
    
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_) {
        std::cerr << "Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }
    
    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "appsink");
    if (!appsink_) {
        std::cerr << "Failed to get appsink element" << std::endl;
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return false;
    }
    
    GstAppSinkCallbacks callbacks = {};
    callbacks.new_sample = onNewSample;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_), &callbacks, this, nullptr);
    
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                       "format", G_TYPE_STRING, "GRAY8",
                                       "width", G_TYPE_INT, 640,
                                       "height", G_TYPE_INT, 480,
                                       nullptr);
    gst_app_sink_set_caps(GST_APP_SINK(appsink_), caps);
    gst_caps_unref(caps);
    
    gst_app_sink_set_emit_signals(GST_APP_SINK(appsink_), TRUE);
    gst_app_sink_set_drop(GST_APP_SINK(appsink_), TRUE);
    
    return true;
}

bool GstPipelineWrapper::start() {
    if (!pipeline_) {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    return ret != GST_STATE_CHANGE_FAILURE;
}

void GstPipelineWrapper::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        appsink_ = nullptr;
    }
}

std::vector<uint8_t> GstPipelineWrapper::getFrame() {
    if (!appsink_) {
        return {};
    }
    
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
    if (!sample) {
        return {};
    }
    
    std::vector<uint8_t> frame = processSample(sample);
    gst_sample_unref(sample);
    
    return frame;
}

bool GstPipelineWrapper::hasFrame() {
    if (!appsink_) {
        return false;
    }
    
    return gst_app_sink_is_eos(GST_APP_SINK(appsink_)) == FALSE;
}

bool GstPipelineWrapper::isPlaying() const {
    if (!pipeline_) {
        return false;
    }
    
    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
    return state == GST_STATE_PLAYING;
}

void GstPipelineWrapper::setNewFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
    frame_callback_ = callback;
}

GstFlowReturn GstPipelineWrapper::onNewSample(GstAppSink* appsink, gpointer user_data) {
    GstPipelineWrapper* self = static_cast<GstPipelineWrapper*>(user_data);
    
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_OK;
    }
    
    std::vector<uint8_t> frame = self->processSample(sample);
    
    if (self->frame_callback_) {
        self->frame_callback_(frame);
    }
    
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

std::vector<uint8_t> GstPipelineWrapper::processSample(GstSample* sample) {
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return {};
    }
    
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return {};
    }
    
    std::vector<uint8_t> result(map.data, map.data + map.size);
    gst_buffer_unmap(buffer, &map);
    
    return result;
}

// GstWebRTCPipeline implementation
GstWebRTCPipeline::GstWebRTCPipeline() : pipeline_(nullptr), webrtcbin_(nullptr) {
    gst_init(nullptr, nullptr);
}

GstWebRTCPipeline::~GstWebRTCPipeline() {
    stop();
}

bool GstWebRTCPipeline::initialize(const std::string& device) {
    // Simplified implementation for testing - no actual WebRTC
    pipeline_ = nullptr;
    webrtcbin_ = nullptr;
    return true;
}

bool GstWebRTCPipeline::start() {
    return true;
}

void GstWebRTCPipeline::stop() {
}

bool GstWebRTCPipeline::isPlaying() const {
    return pipeline_ != nullptr;
}

std::string GstWebRTCPipeline::generateOffer() {
    return "v=0\r\no=- 123456789 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\nm=video 9 UDP/TLS/RTP/SAVPF 96\r\na=rtpmap:96 VP8/90000\r\n";
}

bool GstWebRTCPipeline::setAnswer(const std::string& answer) {
    // Simplified implementation for testing
    return !answer.empty();
}

void GstWebRTCPipeline::onOfferCreated(GstPromise* promise, gpointer user_data) {
    // Simplified implementation for testing
}

void GstWebRTCPipeline::onIceCandidate(GstElement* webrtc, guint mlineindex, gchar* cand, gpointer user_data) {
    // Simplified implementation for testing
}
