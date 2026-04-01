#include "gstreamer_pipeline.hpp"
#include <iostream>
#include <gst/video/video.h>

GstPipeline::GstPipeline() : pipeline_(nullptr), appsink_(nullptr) {
    gst_init(nullptr, nullptr);
}

GstPipeline::~GstPipeline() {
    stop();
}

bool GstPipeline::initialize(const std::string& pipeline_str) {
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

bool GstPipeline::start() {
    if (!pipeline_) {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return false;
    }
    
    return true;
}

void GstPipeline::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        appsink_ = nullptr;
    }
}

bool GstPipeline::isPlaying() const {
    if (!pipeline_) {
        return false;
    }
    
    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);
    return state == GST_STATE_PLAYING;
}

std::vector<uint8_t> GstPipeline::getFrame() {
    if (!appsink_) {
        return std::vector<uint8_t>();
    }
    
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink_));
    if (!sample) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> frame = processSample(sample);
    gst_sample_unref(sample);
    
    return frame;
}

bool GstPipeline::hasFrame() {
    if (!appsink_) {
        return false;
    }
    
    return gst_app_sink_is_eos(GST_APP_SINK(appsink_)) == FALSE;
}

void GstPipeline::setNewFrameCallback(std::function<void(const std::vector<uint8_t>&)> callback) {
    frame_callback_ = callback;
}

GstFlowReturn GstPipeline::onNewSample(GstAppSink* appsink, gpointer user_data) {
    GstPipeline* self = static_cast<GstPipeline*>(user_data);
    
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

std::vector<uint8_t> GstPipeline::processSample(GstSample* sample) {
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        return std::vector<uint8_t>();
    }
    
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> frame_data;
    frame_data.assign(map.data, map.data + map.size);
    
    gst_buffer_unmap(buffer, &map);
    
    return frame_data;
}

// WebRTC Pipeline Implementation
GstWebRTCPipeline::GstWebRTCPipeline() : pipeline_(nullptr), webrtcbin_(nullptr) {
    gst_init(nullptr, nullptr);
}

GstWebRTCPipeline::~GstWebRTCPipeline() {
    stop();
}

bool GstWebRTCPipeline::initialize(const std::string& device) {
    std::string pipeline_str = 
        "v4l2src device=" + device + " ! "
        "videoconvert ! "
        "video/x-raw,width=640,height=480,framerate=30/1 ! "
        "x264enc bitrate=2000 speed-preset=ultrafast tune=zerolatency ! "
        "rtph264pay config-interval=1 pt=96 ! "
        "application/x-rtp,media=video,encoding-name=H264,payload=96 ! "
        "webrtcbin name=sendrecv stun-server=stun://stun.l.google.com:19302";
    
    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_) {
        std::cerr << "Failed to create WebRTC pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }
    
    webrtcbin_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sendrecv");
    if (!webrtcbin_) {
        std::cerr << "Failed to get webrtcbin element" << std::endl;
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        return false;
    }
    
    g_signal_connect(webrtcbin_, "on-ice-candidate", G_CALLBACK(onIceCandidate), this);
    
    return true;
}

bool GstWebRTCPipeline::start() {
    if (!pipeline_) {
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start WebRTC pipeline" << std::endl;
        return false;
    }
    
    return true;
}

void GstWebRTCPipeline::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
        webrtcbin_ = nullptr;
    }
}

std::string GstWebRTCPipeline::generateOffer() {
    if (!webrtcbin_) {
        return "";
    }
    
    GstPromise* promise = gst_promise_new();
    g_signal_emit_by_name(webrtcbin_, "create-offer", nullptr, promise);
    
    gst_promise_wait(promise);
    
    GstStructure* reply = gst_promise_get_reply(promise);
    if (!reply) {
        gst_promise_unref(promise);
        return "";
    }
    
    GstWebRTCSessionDescription* offer = nullptr;
    gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr);
    
    std::string offer_sdp;
    if (offer) {
        gchar* sdp_text = gst_sdp_message_as_text(offer->sdp);
        offer_sdp = sdp_text;
        g_free(sdp_text);
        gst_webrtc_session_description_free(offer);
    }
    
    gst_structure_free(reply);
    gst_promise_unref(promise);
    
    return offer_sdp;
}

bool GstWebRTCPipeline::setAnswer(const std::string& answer) {
    if (!webrtcbin_) {
        return false;
    }
    
    GstSDPMessage* sdp = nullptr;
    if (gst_sdp_message_new_from_text(answer.c_str(), &sdp) != GST_SDP_OK) {
        return false;
    }
    
    GstWebRTCSessionDescription* desc = gst_webrtc_session_description_new(
        GST_WEBRTC_SDP_TYPE_ANSWER, sdp);
    if (!desc) {
        gst_sdp_message_free(sdp);
        return false;
    }
    
    GstPromise* promise = gst_promise_new();
    g_signal_emit_by_name(webrtcbin_, "set-remote-description", desc, promise);
    gst_promise_wait(promise);
    gst_promise_unref(promise);
    
    gst_webrtc_session_description_free(desc);
    
    return true;
}

void GstWebRTCPipeline::onOfferCreated(GstPromise* promise, gpointer user_data) {
    // Handle offer creation callback
}

void GstWebRTCPipeline::onIceCandidate(GstElement* webrtc, guint mlineindex, gchar* cand, gpointer user_data) {
    // Handle ICE candidate callback
    std::cout << "ICE Candidate: " << cand << std::endl;
}
