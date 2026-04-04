#include <gtest/gtest.h>
#include "common/types.h"
#include "common/smart_monitor_protocol.h"
#include "ffi/rust_bridge.h"
#include <cstdint>

// Helper function to get timestamp
static uint32_t get_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Test semantic type system integration
class SemanticTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test environment setup
        setenv("SMART_MONITOR_TEST_MODE", "1", 1);
    }

    void TearDown() override {
        unsetenv("SMART_MONITOR_TEST_MODE");
    }
};

TEST_F(SemanticTypesTest, SemanticTypeSizes) {
    // Test that semantic types have correct sizes
    EXPECT_EQ(sizeof(FrameWidth), sizeof(uint32_t));
    EXPECT_EQ(sizeof(FrameHeight), sizeof(uint32_t));
    EXPECT_EQ(sizeof(ByteCount), sizeof(uint32_t));
    EXPECT_EQ(sizeof(TemperatureC), sizeof(float));
    EXPECT_EQ(sizeof(HumidityPercent), sizeof(float));
    EXPECT_EQ(sizeof(SampleRate), sizeof(uint32_t));
    EXPECT_EQ(sizeof(AudioBuffer), sizeof(uint8_t*));
}

TEST_F(SemanticTypesTest, ProtocolHeaderIntegrity) {
    ProtocolHeader header = {0};
    
    header.magic = PROTOCOL_MAGIC_BYTES;
    header.version = PROTOCOL_VERSION_MAJOR;
    header.type = MSG_TYPE_HEARTBEAT;
    header.sequence = 1;
    header.timestamp = get_timestamp_ms();
    header.payload_length = 0;
    
    EXPECT_EQ(header.magic, PROTOCOL_MAGIC_BYTES);
    EXPECT_EQ(header.version, PROTOCOL_VERSION_MAJOR);
    EXPECT_EQ(header.type, MSG_TYPE_HEARTBEAT);
    EXPECT_EQ(header.sequence, 1);
    EXPECT_GT(header.timestamp, 0);
    EXPECT_EQ(header.payload_length, 0);
}

TEST_F(SemanticTypesTest, SensorDataStructure) {
    SensorData sensor = {0};
    
    sensor.temperature_c = 25.5f;
    sensor.humidity_percent = 60.0f;
    sensor.light_level_lux = 500;
    sensor.pir_motion_detected = true;
    
    EXPECT_FLOAT_EQ(sensor.temperature_c, 25.5f);
    EXPECT_FLOAT_EQ(sensor.humidity_percent, 60.0f);
    EXPECT_EQ(sensor.light_level_lux, 500);
    EXPECT_TRUE(sensor.pir_motion_detected);
}

TEST_F(SemanticTypesTest, AudioDataStructure) {
    AudioData audio = {0};
    
    audio.noise_level = 0.5f;
    audio.voice_activity = true;
    audio.baby_crying = false;
    audio.screaming = false;
    audio.peak_frequency_hz = 1000;
    
    EXPECT_FLOAT_EQ(audio.noise_level, 0.5f);
    EXPECT_TRUE(audio.voice_activity);
    EXPECT_FALSE(audio.baby_crying);
    EXPECT_FALSE(audio.screaming);
    EXPECT_EQ(audio.peak_frequency_hz, 1000);
}

TEST_F(SemanticTypesTest, MotionResultStructure) {
    MotionResult result = {0};
    
    result.motion_detected = true;
    result.motion_level = 0.8f;
    result.changed_pixels = 1000;
    
    EXPECT_TRUE(result.motion_detected);
    EXPECT_FLOAT_EQ(result.motion_level, 0.8f);
    EXPECT_EQ(result.changed_pixels, 1000);
}

TEST_F(SemanticTypesTest, TypeConstants) {
    EXPECT_EQ(DEFAULT_FRAME_WIDTH, 640);
    EXPECT_EQ(DEFAULT_FRAME_HEIGHT, 480);
    EXPECT_EQ(DEFAULT_MOTION_THRESHOLD, 25);
    EXPECT_EQ(DEFAULT_AUDIO_SAMPLE_RATE, 44100);
    EXPECT_STREQ(DEFAULT_VIDEO_DEVICE, "/dev/video0");
    EXPECT_EQ(DEFAULT_HTTP_PORT, 8080);
}

TEST_F(SemanticTypesTest, AudioThresholdConstants) {
    EXPECT_FLOAT_EQ(AUDIO_BABY_CRYING_THRESHOLD, 0.7f);
    EXPECT_FLOAT_EQ(AUDIO_SCREAMING_THRESHOLD, 0.9f);
    EXPECT_FLOAT_EQ(AUDIO_VOICE_ACTIVITY_THRESHOLD, 0.3f);
}

TEST_F(SemanticTypesTest, ProtocolConstants) {
    EXPECT_EQ(PROTOCOL_VERSION_MAJOR, 1);
    EXPECT_EQ(PROTOCOL_VERSION_MINOR, 0);
    EXPECT_EQ(PROTOCOL_MAGIC_BYTES, 0x534D);
}

TEST_F(SemanticTypesTest, TypeSafetyValidation) {
    // Test type safety in assignments
    FrameWidth width = DEFAULT_FRAME_WIDTH;
    FrameHeight height = DEFAULT_FRAME_HEIGHT;
    ByteCount size = width * height * 2; // YUYV format
    
    EXPECT_EQ(width, 640);
    EXPECT_EQ(height, 480);
    EXPECT_EQ(size, 640 * 480 * 2);
    
    // Test audio type safety
    SampleRate sample_rate = DEFAULT_AUDIO_SAMPLE_RATE;
    AudioBuffer buffer = nullptr;
    SampleCount samples = 1024;
    
    EXPECT_EQ(sample_rate, 44100);
    EXPECT_EQ(samples, 1024);
    EXPECT_EQ(buffer, nullptr);
}

// Test environment setup
class TypeSystemTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        setenv("SMART_MONITOR_TEST_MODE", "1", 1);
        setenv("SMART_MONITOR_MOCK_MODE", "1", 1);
    }
    
    void TearDown() override {
        unsetenv("SMART_MONITOR_TEST_MODE");
        unsetenv("SMART_MONITOR_MOCK_MODE");
    }
};
