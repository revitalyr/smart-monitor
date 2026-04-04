#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "audio/audio_capture.h"
#include "common/smart_monitor_constants.h"

class AudioCaptureTest : public ::testing::Test {
protected:
    void SetUp() override {
        audio = audio_create("/dev/dsp0");
        ASSERT_NE(audio, nullptr);
    }

    void TearDown() override {
        if (audio) {
            audio_destroy(audio);
        }
    }

    audio_capture_t* audio;
};

TEST_F(AudioCaptureTest, CreateAndDestroy) {
    // Test basic creation and destruction
    EXPECT_TRUE(audio->initialized);
    EXPECT_FALSE(audio->capturing);
}

TEST_F(AudioCaptureTest, InitializeWithValidParameters) {
    bool result = audio_initialize(audio, 44100, 2);
    EXPECT_TRUE(result);
    EXPECT_EQ(audio->sample_rate, 44100);
    EXPECT_EQ(audio->channels, 2);
}

TEST_F(AudioCaptureTest, InitializeWithInvalidParameters) {
    bool result = audio_initialize(audio, -1, 0);
    EXPECT_FALSE(result);
}

TEST_F(AudioCaptureTest, StartAndStopCapture) {
    audio_initialize(audio, 44100, 2);
    
    bool start_result = audio_start_capture(audio);
    EXPECT_TRUE(start_result);
    EXPECT_TRUE(audio->capturing);
    
    audio_stop_capture(audio);
    EXPECT_FALSE(audio->capturing);
}

TEST_F(AudioCaptureTest, ReadSamplesWhenNotCapturing) {
    uint8_t buffer[1024];
    int bytes_read = audio_read_samples(audio, buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, -1);
}

TEST_F(AudioCaptureTest, AnalyzeSamples) {
    uint8_t test_samples[1024] = {0};
    
    // Generate sine wave test data
    for (int i = 0; i < 512; i++) {
        test_samples[i] = (uint8_t)(127 * sin(2 * M_PI * 440 * i / 44100) + 128);
    }
    
    audio_metrics_t metrics = audio_analyze_samples(test_samples, 512);
    
    EXPECT_GE(metrics.noise_level, 0.0f);
    EXPECT_LE(metrics.noise_level, 1.0f);
    EXPECT_GE(metrics.peak_level, 0.0f);
    EXPECT_LE(metrics.peak_level, 1.0f);
    EXPECT_EQ(metrics.sample_count, 512);
}

TEST_F(AudioCaptureTest, CalculateRMS) {
    uint8_t test_samples[] = {128, 255, 0, 128, 255, 0}; // Sine wave samples
    float rms = audio_calculate_rms(test_samples, sizeof(test_samples));
    
    EXPECT_GT(rms, 0.0f);
    EXPECT_LE(rms, 1.0f);
}

TEST_F(AudioCaptureTest, DetectVoiceActivity) {
    uint8_t silent_samples[512] = {128}; // Silent audio
    uint8_t voice_samples[512];
    
    // Generate voice-like signal
    for (int i = 0; i < 512; i++) {
        voice_samples[i] = (uint8_t)(127 * sin(2 * M_PI * 440 * i / 44100) + 128);
    }
    
    bool silent_detected = audio_detect_voice_activity(silent_samples, 512, 0.1f);
    bool voice_detected = audio_detect_voice_activity(voice_samples, 512, 0.1f);
    
    EXPECT_FALSE(silent_detected);
    EXPECT_TRUE(voice_detected);
}

class AudioCaptureMockTest : public ::testing::Test {
protected:
    void SetUp() override {
        audio = audio_create_mock();
        ASSERT_NE(audio, nullptr);
    }

    void TearDown() override {
        if (audio) {
            audio_destroy(audio);
        }
    }

    audio_capture_t* audio;
};

TEST_F(AudioCaptureMockTest, MockGeneration) {
    uint8_t buffer[1024];
    audio_generate_mock_samples(audio, buffer, sizeof(buffer));
    
    // Check that mock data is generated
    bool has_data = false;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] != 0) {
            has_data = true;
            break;
        }
    }
    EXPECT_TRUE(has_data);
}
