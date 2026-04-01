#include "webrtc_server.hpp"

WebRTCServer::WebRTCServer() {
}

WebRTCServer::~WebRTCServer() {
    stop();
}

bool WebRTCServer::initialize(const std::string& device) {
    current_device_ = device;
    
    webrtc_pipeline_ = std::make_unique<GstWebRTCPipeline>();
    if (!webrtc_pipeline_->initialize(device)) {
        std::cerr << "Failed to initialize WebRTC pipeline" << std::endl;
        return false;
    }
    
    return true;
}

bool WebRTCServer::start() {
    if (!webrtc_pipeline_) {
        return false;
    }
    
    return webrtc_pipeline_->start();
}

void WebRTCServer::stop() {
    if (webrtc_pipeline_) {
        webrtc_pipeline_->stop();
        webrtc_pipeline_.reset();
    }
}

std::string WebRTCServer::generateOffer() {
    if (!webrtc_pipeline_) {
        return "";
    }
    
    return webrtc_pipeline_->generateOffer();
}

bool WebRTCServer::setAnswer(const std::string& answer) {
    if (!webrtc_pipeline_) {
        return false;
    }
    
    return webrtc_pipeline_->setAnswer(answer);
}
