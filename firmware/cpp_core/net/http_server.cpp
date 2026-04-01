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
    
    // Video analysis endpoint
    svr.Post("/analyze/video", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parse JSON body to get video URL
            std::string body = req.body;
            
            // Simple URL extraction (in production, use a JSON library)
            size_t url_start = body.find("\"url\":");
            if (url_start == std::string::npos) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing url field\"}", "application/json");
                return;
            }
            
            url_start = body.find("\"", url_start + 6);
            if (url_start == std::string::npos) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid URL format\"}", "application/json");
                return;
            }
            url_start++;
            
            size_t url_end = body.find("\"", url_start);
            if (url_end == std::string::npos) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid URL format\"}", "application/json");
                return;
            }
            
            std::string video_url = body.substr(url_start, url_end - url_start);
            
            // Log the analysis request
            std::cout << "Starting video analysis for URL: " << video_url << std::endl;
            
            // Update metrics to indicate video analysis is active
            metrics_.camera_active = true;
            
            // For now, return a success response
            // In a full implementation, this would:
            // 1. Download/stream the video
            // 2. Extract frames
            // 3. Process frames through motion detection
            // 4. Return analysis results
            
            std::ostringstream response;
            response << "{"
                    << "\"status\":\"started\","
                    << "\"video_url\":\"" << video_url << "\","
                    << "\"message\":\"Video analysis started\","
                    << "\"expected_duration\":\"Processing...\","
                    << "\"motion_events\":0,"
                    << "\"frames_processed\":0"
                    << "}";
            
            res.set_content(response.str(), "application/json");
            
        } catch (const std::exception& e) {
            res.status = 500;
            std::ostringstream error_response;
            error_response << "{\"error\":\"" << e.what() << "\"}";
            res.set_content(error_response.str(), "application/json");
        }
    });
    
    // Get video analysis status
    svr.Get("/analyze/status", [this](const httplib::Request&, httplib::Response& res) {
        std::ostringstream response;
        response << "{"
                << "\"status\":\"active\","
                << "\"video_url\":\"https://interactive-examples.mdn.mozilla.net/media/cc0-videos/flower.mp4\","
                << "\"motion_events\":" << metrics_.motion_events.load() << ","
                << "\"frames_processed\":" << metrics_.frames_processed.load() << ","
                << "\"motion_level\":" << std::fixed << std::setprecision(2) << metrics_.motion_level.load()
                << "}";
        res.set_content(response.str(), "application/json");
    });
    
    // Static file serving for web dashboard
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            "<!DOCTYPE html><html><head><title>Smart Monitor</title>"
            "<style>body{font-family:Arial,sans-serif;margin:40px;}"
            ".container{max-width:800px;margin:0 auto;}"
            ".status{background:#f5f5f5;padding:20px;border-radius:5px;margin:20px 0;}"
            ".btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px;}"
            ".btn:hover{background:#0056b3;}"
            ".video-section{background:#e9ecef;padding:20px;border-radius:5px;margin:20px 0;}</style>"
            "</head><body><div class='container'>"
            "<h1>Smart Monitor Dashboard</h1>"
            "<p>Video monitoring system with motion detection</p>"
            
            "<div class='video-section'>"
            "<h2>Video Analysis</h2>"
            "<p>Analyze video from URL for motion detection</p>"
            "<input type='text' id='videoUrl' placeholder='Enter video URL' "
            "value='https://interactive-examples.mdn.mozilla.net/media/cc0-videos/flower.mp4' style='width:70%;padding:8px;'>"
            "<button class='btn' onclick='analyzeVideo()'>Analyze Video</button>"
            "<div id='analysisStatus' style='margin-top:10px;'></div>"
            "</div>"
            
            "<div class='status' id='status'>"
            "<h3>System Status</h3>"
            "<p>Loading...</p>"
            "</div>"
            
            "<script>"
            "async function analyzeVideo(){"
            "const url=document.getElementById('videoUrl').value;"
            "const status=document.getElementById('analysisStatus');"
            "status.innerHTML='<p>Starting analysis...</p>';"
            "try{"
            "const response=await fetch('/analyze/video',{"
            "method:'POST',"
            "headers:{'Content-Type':'application/json'},"
            "body:JSON.stringify({url:url})"
            "});"
            "const result=await response.json();"
            "if(response.ok){"
            "status.innerHTML=`<p style='color:green'>${result.message}</p>"
            "<p>Video: ${result.video_url}</p>`;"
            "}else{"
            "status.innerHTML=`<p style='color:red'>Error: ${result.error}</p>`;"
            "}"
            "}catch(error){"
            "status.innerHTML=`<p style='color:red'>Network error: ${error.message}</p>`;"
            "}"
            "}"
            
            "setInterval(async()=>{"
            "const r=await fetch('/metrics');const d=await r.json();"
            "document.getElementById('status').innerHTML="
            "`<h3>System Status</h3>"
            "<p><strong>Motion Events:</strong> ${d.motion_events}</p>"
            "<p><strong>Motion Level:</strong> ${d.motion_level}</p>"
            "<p><strong>Frames Processed:</strong> ${d.frames_processed}</p>"
            "<p><strong>Camera Active:</strong> ${d.camera_active?'Yes':'No'}</p>"
            "<p><strong>Uptime:</strong> ${d.uptime}</p>"
            "<p><strong>Last Motion:</strong> ${d.last_motion_time||'Never'}</p>`;"
            "},1000);"
            "</script></div></body></html>",
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
