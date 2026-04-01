#include "http_server.hpp"
#include "../../../external/httplib/httplib.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

HTTPServer::HTTPServer() : running_(false), port_(8080) {
}

HTTPServer::~HTTPServer() {
    stop();
}

bool HTTPServer::start(int port) {
    if (running_) {
        return false;
    }
    
    port_ = port;
    running_ = true;
    
    server_thread_ = std::make_unique<std::thread>(&HTTPServer::serverLoop, this);
    
    return true;
}

void HTTPServer::stop() {
    if (running_) {
        running_ = false;
        if (server_thread_ && server_thread_->joinable()) {
            server_thread_->join();
        }
    }
}

void HTTPServer::setMetricsCallback(std::function<std::string()> callback) {
    metrics_callback_ = callback;
}

void HTTPServer::setHealthCallback(std::function<std::string()> callback) {
    health_callback_ = callback;
}

void HTTPServer::setWebRTCCallback(std::function<std::string()> callback) {
    webrtc_callback_ = callback;
}

void HTTPServer::serverLoop() {
    httplib::Server svr;
    
    // CORS headers
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        
        if (req.method == "OPTIONS") {
            res.status = 200;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    
    // Health endpoint
    svr.Get("/health", [this](const httplib::Request&, httplib::Response& res) {
        if (health_callback_) {
            res.set_content(health_callback_(), "application/json");
        } else {
            res.set_content(generateHealthJSON(), "application/json");
        }
    });
    
    // Metrics endpoint
    svr.Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
        if (metrics_callback_) {
            res.set_content(metrics_callback_(), "application/json");
        } else {
            res.set_content(generateMetricsJSON(), "application/json");
        }
    });
    
    // WebRTC SDP endpoint
    svr.Get("/webrtc/offer", [this](const httplib::Request&, httplib::Response& res) {
        if (webrtc_callback_) {
            res.set_content(webrtc_callback_(), "application/sdp");
        } else {
            res.set_content(generateWebRTCSDP(), "application/sdp");
        }
    });
    
    // WebRTC answer endpoint
    svr.Post("/webrtc/answer", [this](const httplib::Request& req, httplib::Response& res) {
        std::string answer = req.body;
        std::cout << "Received WebRTC answer: " << answer.substr(0, 100) << "..." << std::endl;
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });
    
    // Static file serving for web dashboard
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            "<!DOCTYPE html><html><head><title>Smart Monitor</title></head>"
            "<body><h1>Smart Monitor Dashboard</h1>"
            "<p>Video monitoring system with motion detection</p>"
            "<div id='status'>Loading...</div>"
            "<script>setInterval(async()=>{"
            "const r=await fetch('/metrics');const d=await r.json();"
            "document.getElementById('status').innerHTML="
            "`Motion Events: ${d.motion_events}<br>"
            "Motion Level: ${d.motion_level}<br>"
            "Frames: ${d.frames_processed}<br>"
            "Camera: ${d.camera_active?'Active':'Inactive'}<br>"
            "Uptime: ${d.uptime}`;},1000);</script></body></html>",
            "text/html"
        );
    });
    
    std::cout << "HTTP Server starting on port " << port_ << std::endl;
    svr.listen("0.0.0.0", port_);
    std::cout << "HTTP Server stopped" << std::endl;
}

std::string HTTPServer::generateMetricsJSON() const {
    std::ostringstream json;
    json << "{"
         << "\"motion_events\":" << metrics_.motion_events.load() << ","
         << "\"motion_level\":" << std::fixed << std::setprecision(2) << metrics_.motion_level.load() << ","
         << "\"frames_processed\":" << metrics_.frames_processed.load() << ","
         << "\"camera_active\":" << (metrics_.camera_active.load() ? "true" : "false") << ","
         << "\"last_motion_time\":\"" << metrics_.last_motion_time << "\","
         << "\"uptime\":\"" << metrics_.uptime << "\""
         << "}";
    return json.str();
}

std::string HTTPServer::generateHealthJSON() const {
    return "{\"status\":\"ok\",\"service\":\"smart-monitor\",\"version\":\"1.0.0\"}";
}

std::string HTTPServer::generateWebRTCSDP() const {
    return "v=0\r\n"
           "o=- 0 0 IN IP4 127.0.0.1\r\n"
           "s=-\r\n"
           "t=0 0\r\n"
           "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
           "a=rtpmap:96 H264/90000\r\n";
}
