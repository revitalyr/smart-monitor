#include <gtest/gtest.h>
#include "../video/v4l2_capture.hpp"
#include "../ffi/rust_bridge.hpp"
#include "../net/http_server.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        motion_detector = std::make_unique<RustMotionDetector>();
        http_server = std::make_unique<HTTPServer>();
        camera = std::make_unique<V4L2Capture>("/dev/null"); // Use dummy device
    }

    void TearDown() override {
        if (http_server && http_server->isRunning()) {
            http_server->stop();
        }
        if (motion_detector) {
            motion_detector->cleanup();
        }
        motion_detector.reset();
        http_server.reset();
        camera.reset();
    }

    std::unique_ptr<RustMotionDetector> motion_detector;
    std::unique_ptr<HTTPServer> http_server;
    std::unique_ptr<V4L2Capture> camera;
};

TEST_F(IntegrationTest, MotionDetectorWithHTTPServer) {
    // Initialize motion detector
    ASSERT_TRUE(motion_detector->initialize());
    
    // Set up HTTP server with callbacks
    bool motion_detected = false;
    std::atomic<int> frame_count{0};
    
    http_server->setMetricsCallback([&]() -> std::string {
        auto& metrics = http_server->getMetrics();
        return "{\"motion_events\": " + std::to_string(metrics.motion_events.load()) +
               ", \"motion_level\": " + std::to_string(metrics.motion_level.load()) +
               ", \"frames_processed\": " + std::to_string(metrics.frames_processed.load()) + "}";
    });
    
    http_server->setHealthCallback([&]() -> std::string {
        return "{\"status\": \"healthy\", \"camera_active\": " + 
               std::string(http_server->getMetrics().camera_active.load() ? "true" : "false") + "}";
    });
    
    // Start HTTP server
    ASSERT_TRUE(http_server->start(8090));
    EXPECT_TRUE(http_server->isRunning());
    
    // Simulate motion detection
    std::vector<uint8_t> frame1(640 * 480, 0);
    std::vector<uint8_t> frame2(640 * 480, 0);
    
    // Add some motion
    for (size_t i = 0; i < frame2.size() / 20; ++i) { // 5% motion
        frame2[i] = 100;
    }
    
    // Detect motion and update metrics
    auto result = motion_detector->detectMotionAdvanced(
        frame1.data(), frame2.data(), 640, 480, 20
    );
    
    if (result.motion_detected) {
        motion_detected = true;
        auto& metrics = http_server->getMetrics();
        metrics.motion_events++;
        metrics.motion_level = result.motion_level;
    }
    
    frame_count++;
    http_server->getMetrics().frames_processed = frame_count.load();
    
    EXPECT_TRUE(motion_detected);
    EXPECT_GT(result.motion_level, 0.0f);
    EXPECT_GT(result.changed_pixels, 0);
    
    http_server->stop();
}

TEST_F(IntegrationTest, FullSystemSimulation) {
    // Initialize all components
    ASSERT_TRUE(motion_detector->initialize());
    
    // Set up comprehensive callbacks
    http_server->setMetricsCallback([&]() -> std::string {
        auto& metrics = http_server->getMetrics();
        return "{"
               "\"motion_events\": " + std::to_string(metrics.motion_events.load()) + ","
               "\"motion_level\": " + std::to_string(metrics.motion_level.load()) + ","
               "\"frames_processed\": " + std::to_string(metrics.frames_processed.load()) + ","
               "\"camera_active\": " + std::string(metrics.camera_active.load() ? "true" : "false") + ","
               "\"last_motion_time\": \"" + metrics.last_motion_time + "\","
               "\"uptime\": \"" + metrics.uptime + "\""
               "}";
    });
    
    // Start HTTP server
    ASSERT_TRUE(http_server->start(8091));
    
    // Simulate camera activity
    auto& metrics = http_server->getMetrics();
    metrics.camera_active = true;
    metrics.uptime = "0h 1m 30s";
    
    // Simulate frame processing
    std::vector<uint8_t> prev_frame(640 * 480, 128);
    std::vector<uint8_t> curr_frame(640 * 480, 128);
    
    for (int i = 0; i < 10; ++i) {
        // Simulate some motion in some frames
        if (i % 3 == 0) {
            // Add motion to random areas
            for (int j = 0; j < 1000; ++j) {
                size_t idx = (i * 1000 + j) % curr_frame.size();
                curr_frame[idx] = 200;
            }
        } else {
            // No motion - copy previous frame
            std::copy(prev_frame.begin(), prev_frame.end(), curr_frame.begin());
        }
        
        // Detect motion
        auto result = motion_detector->detectMotionAdvanced(
            prev_frame.data(), curr_frame.data(), 640, 480, 15
        );
        
        // Update metrics
        metrics.frames_processed++;
        
        if (result.motion_detected) {
            metrics.motion_events++;
            metrics.motion_level = result.motion_level;
            metrics.last_motion_time = "2023-01-01 12:00:" + std::to_string(i);
        }
        
        // Swap frames for next iteration
        std::swap(prev_frame, curr_frame);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Verify system state
    EXPECT_GT(metrics.frames_processed.load(), 0);
    EXPECT_GT(metrics.motion_events.load(), 0);
    EXPECT_TRUE(metrics.camera_active.load());
    
    http_server->stop();
}

TEST_F(IntegrationTest, ConcurrentMotionDetectionAndHTTP) {
    // Initialize components
    ASSERT_TRUE(motion_detector->initialize());
    ASSERT_TRUE(http_server->start(8092));
    
    auto& metrics = http_server->getMetrics();
    std::atomic<bool> stop_simulation{false};
    
    // Motion detection thread
    std::thread motion_thread([&]() {
        std::vector<uint8_t> frame1(320 * 240, 0);
        std::vector<uint8_t> frame2(320 * 240, 0);
        
        int frame_counter = 0;
        while (!stop_simulation.load()) {
            // Simulate changing frames
            for (size_t i = 0; i < frame2.size(); ++i) {
                frame2[i] = static_cast<uint8_t>((frame_counter + i) % 256);
            }
            
            auto result = motion_detector->detectMotionAdvanced(
                frame1.data(), frame2.data(), 320, 240, 10
            );
            
            if (result.motion_detected) {
                metrics.motion_events++;
                metrics.motion_level = result.motion_level;
            }
            
            metrics.frames_processed++;
            frame_counter++;
            
            std::swap(frame1, frame2);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    // HTTP server metrics update thread
    std::thread metrics_thread([&]() {
        while (!stop_simulation.load()) {
            // Simulate periodic metrics updates
            metrics.camera_active = true;
            metrics.uptime = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count()) + "s";
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
    
    // Let simulation run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop_simulation = true;
    
    motion_thread.join();
    metrics_thread.join();
    
    // Verify results
    EXPECT_GT(metrics.frames_processed.load(), 0);
    EXPECT_TRUE(metrics.camera_active.load());
    
    http_server->stop();
}

TEST_F(IntegrationTest, ErrorHandlingAndRecovery) {
    // Test system behavior with error conditions
    
    // Initialize with dummy camera (will fail but shouldn't crash)
    EXPECT_FALSE(camera->initialize(640, 480));
    EXPECT_FALSE(camera->isInitialized());
    
    // Motion detector should still work
    ASSERT_TRUE(motion_detector->initialize());
    
    // HTTP server should start even with camera issues
    ASSERT_TRUE(http_server->start(8093));
    
    // Simulate error recovery
    auto& metrics = http_server->getMetrics();
    metrics.camera_active = false;
    
    // Simulate motion detection with invalid data
    std::vector<uint8_t> empty_frame;
    auto result = motion_detector->detectMotionAdvanced(
        nullptr, nullptr, 0, 0, 10
    );
    
    EXPECT_FALSE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 0);
    EXPECT_FLOAT_EQ(result.motion_level, 0.0f);
    
    // System should remain functional
    EXPECT_TRUE(http_server->isRunning());
    
    http_server->stop();
}

TEST_F(IntegrationTest, PerformanceMetrics) {
    // Test system performance under load
    ASSERT_TRUE(motion_detector->initialize());
    ASSERT_TRUE(http_server->start(8094));
    
    const int num_frames = 100;
    const int frame_width = 640;
    const int frame_height = 480;
    const size_t frame_size = frame_width * frame_height;
    
    std::vector<uint8_t> frame1(frame_size, 0);
    std::vector<uint8_t> frame2(frame_size, 0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_frames; ++i) {
        // Simulate frame changes
        for (size_t j = 0; j < frame_size; j += 100) {
            frame2[j] = static_cast<uint8_t>(i % 256);
        }
        
        auto result = motion_detector->detectMotionAdvanced(
            frame1.data(), frame2.data(), frame_width, frame_height, 20
        );
        
        auto& metrics = http_server->getMetrics();
        metrics.frames_processed++;
        
        if (result.motion_detected) {
            metrics.motion_events++;
            metrics.motion_level = result.motion_level;
        }
        
        std::swap(frame1, frame2);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto& metrics = http_server->getMetrics();
    EXPECT_EQ(metrics.frames_processed.load(), num_frames);
    
    // Performance should be reasonable (less than 1 second for 100 frames)
    EXPECT_LT(duration.count(), 1000);
    
    // Calculate FPS
    double fps = (double)num_frames / (duration.count() / 1000.0);
    EXPECT_GT(fps, 10.0); // Should handle at least 10 FPS
    
    http_server->stop();
}
