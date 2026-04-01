#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../video/v4l2_capture.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>

class V4L2CaptureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a dummy device path for testing
        capture = std::make_unique<V4L2Capture>("/dev/null");
    }

    void TearDown() override {
        capture.reset();
    }

    std::unique_ptr<V4L2Capture> capture;
};

TEST_F(V4L2CaptureTest, ConstructorWithDefaultDevice) {
    V4L2Capture default_capture;
    EXPECT_FALSE(default_capture.isInitialized());
    EXPECT_EQ(default_capture.getWidth(), 640);
    EXPECT_EQ(default_capture.getHeight(), 480);
    EXPECT_EQ(default_capture.getFrameSize(), 0);
}

TEST_F(V4L2CaptureTest, ConstructorWithCustomDevice) {
    V4L2Capture custom_capture("/dev/video1");
    EXPECT_FALSE(custom_capture.isInitialized());
}

TEST_F(V4L2CaptureTest, InitializeWithInvalidDevice) {
    // This should fail with /dev/null
    bool result = capture->initialize(640, 480);
    EXPECT_FALSE(result);
    EXPECT_FALSE(capture->isInitialized());
}

TEST_F(V4L2CaptureTest, InitializeWithValidDimensions) {
    // Test with different dimensions
    V4L2Capture test_capture("/dev/null");
    
    bool result = test_capture.initialize(320, 240);
    EXPECT_FALSE(result); // Will fail with /dev/null but dimensions should be set
    
    result = test_capture.initialize(1920, 1080);
    EXPECT_FALSE(result); // Will fail with /dev/null
}

TEST_F(V4L2CaptureTest, StartCaptureWithoutInitialization) {
    // Should not crash when starting capture without proper initialization
    EXPECT_NO_THROW(capture->startCapture());
    EXPECT_NO_THROW(capture->stopCapture());
}

TEST_F(V4L2CaptureTest, ReadFrameWithoutInitialization) {
    // Should return empty frame when not initialized
    auto frame = capture->readFrame();
    EXPECT_TRUE(frame.empty());
    
    // readFrameRaw returns a dummy frame even when not initialized
    auto raw_frame = capture->readFrameRaw();
    // It should return a frame of default size (640*480*2) filled with 128
    EXPECT_EQ(raw_frame.size(), 640 * 480 * 2);
    EXPECT_TRUE(std::all_of(raw_frame.begin(), raw_frame.end(), [](uint8_t val) { return val == 128; }));
}

TEST_F(V4L2CaptureTest, DefaultDimensions) {
    V4L2Capture default_capture;
    EXPECT_EQ(default_capture.getWidth(), 640);
    EXPECT_EQ(default_capture.getHeight(), 480);
}

TEST_F(V4L2CaptureTest, InitializationStateConsistency) {
    V4L2Capture test_capture;
    
    // Initially not initialized
    EXPECT_FALSE(test_capture.isInitialized());
    EXPECT_EQ(test_capture.getFrameSize(), 0);
    
    // Attempt initialization (will fail with /dev/null)
    test_capture.initialize(800, 600);
    
    // Should still not be initialized
    EXPECT_FALSE(test_capture.isInitialized());
}

// Mock test for V4L2Buffer structure
TEST(V4L2BufferTest, BufferStructure) {
    V4L2Buffer buffer;
    buffer.start = nullptr;
    buffer.length = 0;
    
    EXPECT_EQ(buffer.start, nullptr);
    EXPECT_EQ(buffer.length, 0);
}

// Integration-style test with multiple instances
TEST_F(V4L2CaptureTest, MultipleInstances) {
    V4L2Capture capture1("/dev/null");
    V4L2Capture capture2("/dev/video0");
    
    // Both should start uninitialized
    EXPECT_FALSE(capture1.isInitialized());
    EXPECT_FALSE(capture2.isInitialized());
    
    // Both should handle initialization gracefully
    EXPECT_NO_THROW(capture1.initialize(640, 480));
    EXPECT_NO_THROW(capture2.initialize(320, 240));
}
