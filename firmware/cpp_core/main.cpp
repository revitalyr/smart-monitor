#include "video/v4l2_capture.hpp"
#include "video/gstreamer_pipeline.hpp"
#include "ffi/rust_bridge.hpp"
#include "net/http_server.hpp"
#include "net/webrtc_server.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <iomanip>
#include <sstream>

volatile sig_atomic_t running = 1;

void signalHandler(int sig) {
    running = 0;
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string calculateUptime(const std::chrono::steady_clock::time_point& start) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);
    
    int hours = duration.count() / 3600;
    int minutes = (duration.count() % 3600) / 60;
    int seconds = duration.count() % 60;
    
    std::ostringstream oss;
    oss << hours << "h " << minutes << "m " << seconds << "s";
    return oss.str();
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "Smart Monitor starting..." << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Initialize components
    V4L2Capture camera;
    RustMotionDetector motion_detector;
    HTTPServer http_server;
    WebRTCServer webrtc_server;
    
    // Initialize camera
    if (!camera.initialize(640, 480)) {
        std::cerr << "Failed to initialize camera, using mock mode" << std::endl;
    } else {
        std::cout << "Camera initialized successfully" << std::endl;
    }
    
    // Initialize motion detector
    if (!motion_detector.initialize()) {
        std::cerr << "Failed to initialize motion detector" << std::endl;
        return 1;
    }
    
    // Start HTTP server
    if (!http_server.start(8080)) {
        std::cerr << "Failed to start HTTP server" << std::endl;
        return 1;
    }
    
    std::cout << "HTTP server started on port 8080" << std::endl;
    
    // Initialize WebRTC server
    if (webrtc_server.initialize()) {
        std::cout << "WebRTC server initialized" << std::endl;
    }
    
    // Set up HTTP server callbacks
    http_server.setHealthCallback([]() {
        return "{\"status\":\"ok\",\"service\":\"smart-monitor\",\"version\":\"1.0.0\"}";
    });
    
    http_server.setWebRTCCallback([&webrtc_server]() {
        if (webrtc_server.isInitialized()) {
            std::string offer = webrtc_server.generateOffer();
            if (!offer.empty()) {
                return offer;
            }
        }
        return std::string("v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n");
    });
    
    // Start camera capture
    if (camera.isInitialized()) {
        if (!camera.startCapture()) {
            std::cerr << "Failed to start camera capture" << std::endl;
        } else {
            http_server.getMetrics().camera_active = true;
            std::cout << "Camera capture started" << std::endl;
        }
    }
    
    // Main processing loop
    std::vector<uint8_t> prev_frame;
    
    while (running) {
        std::vector<uint8_t> current_frame;
        
        if (camera.isInitialized()) {
            current_frame = camera.readFrameRaw();
        } else {
            // Mock frame data for testing
            current_frame.resize(640 * 480);
            for (size_t i = 0; i < current_frame.size(); ++i) {
                current_frame[i] = static_cast<uint8_t>((i % 256) * (rand() % 2));
            }
        }
        
        if (!current_frame.empty() && !prev_frame.empty()) {
            // Convert to grayscale for motion detection
            std::vector<uint8_t> gray_current(current_frame.size() / 2);
            std::vector<uint8_t> gray_prev(prev_frame.size() / 2);
            
            for (size_t i = 0, j = 0; i < current_frame.size(); i += 4, ++j) {
                gray_current[j] = current_frame[i];
                gray_prev[j] = prev_frame[i];
            }
            
            // Detect motion using Rust module
            MotionResult result = motion_detector.detectMotionAdvanced(
                gray_prev.data(),
                gray_current.data(),
                640,
                480,
                20
            );
            
            if (result.motion_detected) {
                http_server.getMetrics().motion_events++;
                http_server.getMetrics().motion_level = result.motion_level;
                http_server.getMetrics().last_motion_time = getCurrentTime();
                
                std::cout << "Motion detected! Level: " << result.motion_level 
                          << ", Changed pixels: " << result.changed_pixels << std::endl;
            }
            
            http_server.getMetrics().frames_processed++;
        }
        
        prev_frame = current_frame;
        
        // Update uptime
        http_server.getMetrics().uptime = calculateUptime(start_time);
        
        // Sleep to control frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }
    
    std::cout << "\nShutting down Smart Monitor..." << std::endl;
    
    // Cleanup
    camera.stopCapture();
    motion_detector.cleanup();
    http_server.stop();
    webrtc_server.stop();
    
    std::cout << "Smart Monitor stopped" << std::endl;
    
    return 0;
}
