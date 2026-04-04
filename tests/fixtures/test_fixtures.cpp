#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <chrono>

// Test fixtures and utilities
class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Set up test environment
        setenv("SMART_MONITOR_TEST_MODE", "1", 1);
        setenv("GST_DEBUG", "0", 1); // Reduce GStreamer noise in tests
    }

    void TearDown() override {
        // Clean up test environment
        unsetenv("SMART_MONITOR_TEST_MODE");
        unsetenv("GST_DEBUG");
    }
};

// Test data generators
namespace TestFixtures {
    // Generate test audio data (sine wave)
    void generate_sine_wave(uint8_t* buffer, size_t size, float frequency, int sample_rate) {
        for (size_t i = 0; i < size; i++) {
            float sample = sin(2.0f * M_PI * frequency * i / sample_rate);
            buffer[i] = static_cast<uint8_t>(127.0f * sample + 128.0f);
        }
    }

    // Generate test sensor data
    sensor_data_t generate_sensor_data(float temp, float humidity, uint32_t light) {
        sensor_data_t data;
        data.temperature = temp;
        data.humidity = humidity;
        data.light_level = light;
        data.motion_detected = false;
        data.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        return data;
    }

    // Generate test audio data
    audio_data_t generate_audio_data(float noise_level, float peak_level, bool voice_detected) {
        audio_data_t data;
        data.noise_level = noise_level;
        data.peak_level = peak_level;
        data.voice_detected = voice_detected;
        data.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        return data;
    }

    // Generate test video data
    video_data_t generate_video_data(float motion_prob, float motion_level) {
        video_data_t data;
        data.motion_prob = motion_prob;
        data.motion_level = motion_level;
        data.timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        return data;
    }

    // Verify JSON structure
    bool is_valid_json(const char* json_string) {
        if (!json_string) return false;
        
        // Basic JSON validation
        size_t len = strlen(json_string);
        if (len < 2) return false;
        
        // Should start with { and end with }
        if (json_string[0] != '{' || json_string[len-1] != '}') return false;
        
        // Basic bracket matching
        int brace_count = 0;
        for (size_t i = 0; i < len; i++) {
            if (json_string[i] == '{') brace_count++;
            else if (json_string[i] == '}') brace_count--;
            if (brace_count < 0) return false;
        }
        
        return brace_count == 0;
    }

    // Create temporary test file
    std::string create_temp_file(const std::string& content) {
        std::string temp_path = "/tmp/test_" + std::to_string(std::time(nullptr)) + ".tmp";
        std::ofstream file(temp_path);
        file << content;
        file.close();
        return temp_path;
    }

    // Clean up temporary file
    void cleanup_temp_file(const std::string& path) {
        unlink(path.c_str());
    }

    // Wait for condition with timeout
    bool wait_for_condition(std::function<bool()> condition, int timeout_ms) {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout_ms) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    // Compare floating point values with tolerance
    bool float_equals(float a, float b, float tolerance = 0.001f) {
        return std::abs(a - b) < tolerance;
    }

    // Check if file exists
    bool file_exists(const std::string& path) {
        return access(path.c_str(), F_OK) == 0;
    }

    // Check if device is available
    bool device_available(const std::string& device_path) {
        return access(device_path.c_str(), R_OK | W_OK) == 0;
    }
}

// Mock classes for testing
class MockAudioCapture {
public:
    MockAudioCapture() : initialized_(false), capturing_(false) {}
    
    bool initialize(int sample_rate, int channels) {
        sample_rate_ = sample_rate;
        channels_ = channels;
        initialized_ = true;
        return true;
    }
    
    bool start_capture() {
        if (!initialized_) return false;
        capturing_ = true;
        return true;
    }
    
    void stop_capture() {
        capturing_ = false;
    }
    
    int read_samples(uint8_t* buffer, int size) {
        if (!capturing_) return -1;
        
        // Generate mock data
        TestFixtures::generate_sine_wave(buffer, size, 440.0f, sample_rate_);
        return size;
    }
    
    bool is_initialized() const { return initialized_; }
    bool is_capturing() const { return capturing_; }
    
private:
    bool initialized_;
    bool capturing_;
    int sample_rate_;
    int channels_;
};

class MockI2CSensor {
public:
    MockI2CSensor() : initialized_(false) {}
    
    bool initialize(const std::string& device_path, uint8_t addr) {
        device_path_ = device_path;
        device_addr_ = addr;
        initialized_ = true;
        return true;
    }
    
    sensor_data_t read_data() {
        if (!initialized_) {
            sensor_data_t empty = {0};
            return empty;
        }
        
        // Generate realistic mock data
        static float temp_base = 22.0f;
        static float humidity_base = 45.0f;
        
        temp_base += (rand() % 100 - 50) * 0.01f; // ±0.5°C variation
        humidity_base += (rand() % 100 - 50) * 0.01f; // ±0.5% variation
        
        return TestFixtures::generate_sensor_data(
            temp_base,
            humidity_base,
            rand() % 1000
        );
    }
    
    bool is_initialized() const { return initialized_; }
    
private:
    bool initialized_;
    std::string device_path_;
    uint8_t device_addr_;
};

// Performance test utilities
class PerformanceTimer {
public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        return duration.count() / 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

// Memory leak detection helper
class MemoryLeakDetector {
public:
    MemoryLeakDetector() {
        // Record initial memory usage (Linux specific)
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                initial_memory_ = std::stoi(line.substr(line.find_last_of('\t') + 1));
                break;
            }
        }
    }
    
    size_t get_leaked_bytes() {
        size_t current_memory = 0;
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                current_memory = std::stoi(line.substr(line.find_last_of('\t') + 1));
                break;
            }
        }
        return current_memory > initial_memory_ ? current_memory - initial_memory_ : 0;
    }
    
private:
    size_t initial_memory_;
};
