#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../net/http_server.hpp"
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <atomic>

class HTTPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<HTTPServer>();
    }

    void TearDown() override {
        if (server && server->isRunning()) {
            server->stop();
        }
        server.reset();
    }

    std::unique_ptr<HTTPServer> server;
};

TEST_F(HTTPServerTest, ConstructorAndDestructor) {
    HTTPServer local_server;
    EXPECT_FALSE(local_server.isRunning());
}

TEST_F(HTTPServerTest, StartAndStop) {
    EXPECT_TRUE(server->start(8081)); // Use different port to avoid conflicts
    EXPECT_TRUE(server->isRunning());
    
    server->stop();
    EXPECT_FALSE(server->isRunning());
}

TEST_F(HTTPServerTest, StartOnSamePort) {
    EXPECT_TRUE(server->start(8082));
    EXPECT_TRUE(server->isRunning());
    
    // Starting on same port should fail
    HTTPServer server2;
    EXPECT_FALSE(server2.start(8082));
    
    server->stop();
}

TEST_F(HTTPServerTest, MetricsAccess) {
    auto& metrics = server->getMetrics();
    
    // Initial values
    EXPECT_EQ(metrics.motion_events.load(), 0);
    EXPECT_FLOAT_EQ(metrics.motion_level.load(), 0.0f);
    EXPECT_EQ(metrics.frames_processed.load(), 0);
    EXPECT_FALSE(metrics.camera_active.load());
    
    // Test atomic operations
    metrics.motion_events++;
    metrics.motion_level = 0.5f;
    metrics.frames_processed = 100;
    metrics.camera_active = true;
    
    EXPECT_EQ(metrics.motion_events.load(), 1);
    EXPECT_FLOAT_EQ(metrics.motion_level.load(), 0.5f);
    EXPECT_EQ(metrics.frames_processed.load(), 100);
    EXPECT_TRUE(metrics.camera_active.load());
}

TEST_F(HTTPServerTest, SetCallbacks) {
    bool metrics_called = false;
    bool health_called = false;
    bool webrtc_called = false;
    
    server->setMetricsCallback([&metrics_called]() -> std::string {
        metrics_called = true;
        return "{\"metrics\": \"test\"}";
    });
    
    server->setHealthCallback([&health_called]() -> std::string {
        health_called = true;
        return "{\"status\": \"healthy\"}";
    });
    
    server->setWebRTCCallback([&webrtc_called]() -> std::string {
        webrtc_called = true;
        return "{\"sdp\": \"test\"}";
    });
    
    // Start server to trigger callbacks
    server->start(8083);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server->stop();
}

TEST_F(HTTPServerTest, MetricsDataStructure) {
    MetricsData metrics;
    
    // Test atomic operations
    metrics.motion_events = 42;
    metrics.motion_level = 0.75f;
    metrics.frames_processed = 1000;
    metrics.camera_active = true;
    metrics.last_motion_time = "2023-01-01 12:00:00";
    metrics.uptime = "1h 30m 45s";
    
    EXPECT_EQ(metrics.motion_events.load(), 42);
    EXPECT_FLOAT_EQ(metrics.motion_level.load(), 0.75f);
    EXPECT_EQ(metrics.frames_processed.load(), 1000);
    EXPECT_TRUE(metrics.camera_active.load());
    EXPECT_EQ(metrics.last_motion_time, "2023-01-01 12:00:00");
    EXPECT_EQ(metrics.uptime, "1h 30m 45s");
}

TEST_F(HTTPServerTest, MultipleStartStop) {
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(server->start(8084 + i));
        EXPECT_TRUE(server->isRunning());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        server->stop();
        EXPECT_FALSE(server->isRunning());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

TEST_F(HTTPServerTest, ConcurrentAccess) {
    server->start(8085);
    
    auto& metrics = server->getMetrics();
    std::atomic<bool> stop_flag{false};
    
    // Producer thread
    std::thread producer([&metrics, &stop_flag]() {
        int counter = 0;
        while (!stop_flag.load()) {
            metrics.motion_events++;
            metrics.frames_processed++;
            metrics.motion_level = 0.5f + (counter % 10) * 0.05f;
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Consumer thread
    std::thread consumer([&metrics, &stop_flag]() {
        while (!stop_flag.load()) {
            int events = metrics.motion_events.load();
            int frames = metrics.frames_processed.load();
            float level = metrics.motion_level.load();
            // Just reading to test thread safety
            (void)events; (void)frames; (void)level;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop_flag = true;
    
    producer.join();
    consumer.join();
    
    server->stop();
}

// Mock callback tests
TEST(HTTPServerCallbacksTest, CallbackFunctionality) {
    HTTPServer test_server;
    
    // Test metrics callback
    bool callback_executed = false;
    test_server.setMetricsCallback([&callback_executed]() -> std::string {
        callback_executed = true;
        return "{\"test\": \"metrics\"}";
    });
    
    // Test health callback
    test_server.setHealthCallback([]() -> std::string {
        return "{\"status\": \"ok\"}";
    });
    
    // Test WebRTC callback
    test_server.setWebRTCCallback([]() -> std::string {
        return "{\"sdp\": \"mock_sdp\"}";
    });
    
    // Start server briefly to test callbacks
    test_server.start(8086);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test_server.stop();
}

// Test server with different configurations
TEST(HTTPServerConfigTest, DifferentPorts) {
    std::vector<std::unique_ptr<HTTPServer>> servers;
    std::vector<int> ports = {8087, 8088, 8089};
    
    for (int port : ports) {
        auto server = std::make_unique<HTTPServer>();
        EXPECT_TRUE(server->start(port));
        EXPECT_TRUE(server->isRunning());
        servers.push_back(std::move(server));
    }
    
    // Clean up
    for (auto& server : servers) {
        server->stop();
        EXPECT_FALSE(server->isRunning());
    }
}
