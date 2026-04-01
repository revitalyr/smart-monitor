#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

struct MetricsData {
    std::atomic<int> motion_events{0};
    std::atomic<float> motion_level{0.0f};
    std::atomic<int> frames_processed{0};
    std::atomic<bool> camera_active{false};
    std::string last_motion_time;
    std::string uptime;
};

class HTTPServer {
public:
    HTTPServer();
    ~HTTPServer();
    
    bool start(int port = 8080);
    void stop();
    bool isRunning() const { return running_; }
    
    void setMetricsCallback(std::function<std::string()> callback);
    void setHealthCallback(std::function<std::string()> callback);
    void setWebRTCCallback(std::function<std::string()> callback);
    
    MetricsData& getMetrics() { return metrics_; }

private:
    std::unique_ptr<std::thread> server_thread_;
    std::atomic<bool> running_;
    int port_;
    
    MetricsData metrics_;
    
    std::function<std::string()> metrics_callback_;
    std::function<std::string()> health_callback_;
    std::function<std::string()> webrtc_callback_;
    
    void serverLoop();
    std::string generateMetricsJSON() const;
    std::string generateHealthJSON() const;
    std::string generateWebRTCSDP() const;
};
