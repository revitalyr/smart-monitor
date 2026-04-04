#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

#include <gtest/gtest.h>
#include <string>
#include <functional>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <atomic>
#include <thread>

// Include project headers
#include "audio/audio_capture.h"
#include "sensors/i2c_sensor.h"
#include "agent/data_agent.h"
#include "common/smart_monitor_protocol.h"

// Test environment setup
class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override;
    void TearDown() override;
};

// Test fixtures namespace
namespace TestFixtures {
    // Data generators
    void generate_sine_wave(uint8_t* buffer, size_t size, float frequency, int sample_rate);
    sensor_data_t generate_sensor_data(float temp, float humidity, uint32_t light);
    audio_data_t generate_audio_data(float noise_level, float peak_level, bool voice_detected);
    video_data_t generate_video_data(float motion_prob, float motion_level);
    
    // Validation utilities
    bool is_valid_json(const char* json_string);
    bool float_equals(float a, float b, float tolerance = 0.001f);
    
    // File utilities
    std::string create_temp_file(const std::string& content);
    void cleanup_temp_file(const std::string& path);
    bool file_exists(const std::string& path);
    bool device_available(const std::string& device_path);
    
    // Timing utilities
    bool wait_for_condition(std::function<bool()> condition, int timeout_ms);
    
    // Constants for testing
    constexpr float DEFAULT_FREQUENCY = 440.0f;
    constexpr int DEFAULT_SAMPLE_RATE = 44100;
    constexpr int DEFAULT_CHANNELS = 2;
    constexpr float DEFAULT_MOTION_THRESHOLD = 0.3f;
    constexpr float DEFAULT_AUDIO_THRESHOLD = 0.2f;
    constexpr int DEFAULT_TIMEOUT_MS = 5000;
    
    // Test data ranges
    constexpr float MIN_TEMPERATURE = -40.0f;
    constexpr float MAX_TEMPERATURE = 85.0f;
    constexpr float MIN_HUMIDITY = 0.0f;
    constexpr float MAX_HUMIDITY = 100.0f;
    constexpr uint32_t MAX_LIGHT_LEVEL = 65535;
}

// Mock classes
class MockAudioCapture {
public:
    MockAudioCapture();
    bool initialize(int sample_rate, int channels);
    bool start_capture();
    void stop_capture();
    int read_samples(uint8_t* buffer, int size);
    bool is_initialized() const;
    bool is_capturing() const;
    
private:
    bool initialized_;
    bool capturing_;
    int sample_rate_;
    int channels_;
};

class MockI2CSensor {
public:
    MockI2CSensor();
    bool initialize(const std::string& device_path, uint8_t addr);
    sensor_data_t read_data();
    bool is_initialized() const;
    
private:
    bool initialized_;
    std::string device_path_;
    uint8_t device_addr_;
};

// Performance utilities
class PerformanceTimer {
public:
    void start();
    double elapsed_ms();
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

// Memory leak detection
class MemoryLeakDetector {
public:
    MemoryLeakDetector();
    size_t get_leaked_bytes();
    
private:
    size_t initial_memory_;
};

// Test macros
#define EXPECT_VALID_JSON(json_str) \
    EXPECT_TRUE(TestFixtures::is_valid_json(json_str)) << "Invalid JSON: " << json_str

#define EXPECT_IN_RANGE(value, min_val, max_val) \
    EXPECT_GE(value, min_val) << "Value " << value << " is below minimum " << min_val; \
    EXPECT_LE(value, max_val) << "Value " << value << " is above maximum " << max_val

#define EXPECT_DEVICE_AVAILABLE(device_path) \
    EXPECT_TRUE(TestFixtures::device_available(device_path)) << "Device not available: " << device_path

#define WAIT_FOR_CONDITION(condition, timeout_ms) \
    EXPECT_TRUE(TestFixtures::wait_for_condition(condition, timeout_ms)) << "Condition not met within timeout"

// Test constants
namespace TestConstants {
    constexpr int TEST_TIMEOUT_MS = 5000;
    constexpr int SHORT_TIMEOUT_MS = 1000;
    constexpr int LONG_TIMEOUT_MS = 10000;
    constexpr int TEST_ITERATIONS = 100;
    constexpr float FLOAT_TOLERANCE = 0.001f;
    constexpr float MOTION_TOLERANCE = 0.01f;
    constexpr float AUDIO_TOLERANCE = 0.01f;
    constexpr float TEMPERATURE_TOLERANCE = 0.1f;
    constexpr float HUMIDITY_TOLERANCE = 0.5f;
}

// Test helper functions
namespace TestHelpers {
    // Setup virtual camera for testing
    bool setup_virtual_camera();
    void cleanup_virtual_camera();
    
    // Generate test video file
    std::string generate_test_video(int duration_seconds = 5);
    
    // Check if running in CI environment
    bool is_ci_environment();
    
    // Skip test if hardware not available
    void skip_if_no_hardware(const std::string& device_path);
    
    // Create test directory
    std::string create_test_directory();
    
    // Clean up test directory
    void cleanup_test_directory(const std::string& path);
}

#endif // TEST_FIXTURES_H
