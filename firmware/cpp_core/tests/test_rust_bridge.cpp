#include <gtest/gtest.h>
#include "../ffi/rust_bridge.hpp"
#include <vector>
#include <cstring>

class RustBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = std::make_unique<RustMotionDetector>();
    }

    void TearDown() override {
        if (detector) {
            detector->cleanup();
        }
        detector.reset();
    }

    std::unique_ptr<RustMotionDetector> detector;
};

TEST_F(RustBridgeTest, ConstructorAndDestructor) {
    RustMotionDetector local_detector;
    // Should not crash
}

TEST_F(RustBridgeTest, Initialization) {
    EXPECT_TRUE(detector->initialize());
    EXPECT_TRUE(detector->initialize()); // Should be idempotent
}

TEST_F(RustBridgeTest, Cleanup) {
    detector->initialize();
    EXPECT_NO_THROW(detector->cleanup());
    EXPECT_NO_THROW(detector->cleanup()); // Should be idempotent
}

TEST_F(RustBridgeTest, BasicMotionDetection) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 0);
    
    // No motion initially
    EXPECT_FALSE(detector->detectMotion(frame1.data(), frame2.data(), 100, 10));
    
    // Add motion
    std::fill(frame2.begin(), frame2.end(), 50);
    EXPECT_TRUE(detector->detectMotion(frame1.data(), frame2.data(), 100, 10));
}

TEST_F(RustBridgeTest, AdvancedMotionDetection) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 0);
    
    // No motion
    auto result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 10, 10, 10);
    EXPECT_FALSE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 0);
    EXPECT_FLOAT_EQ(result.motion_level, 0.0f);
    
    // Add motion to some pixels
    for (int i = 0; i < 20; ++i) {
        frame2[i] = 100;
    }
    
    result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 10, 10, 10);
    EXPECT_TRUE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 20);
    EXPECT_GT(result.motion_level, 0.0f);
}

TEST_F(RustBridgeTest, NullPointerHandling) {
    detector->initialize();
    
    // Test with null pointers - should not crash
    EXPECT_FALSE(detector->detectMotion(nullptr, nullptr, 100, 10));
    
    auto result = detector->detectMotionAdvanced(nullptr, nullptr, 10, 10, 10);
    EXPECT_FALSE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 0);
    EXPECT_FLOAT_EQ(result.motion_level, 0.0f);
}

TEST_F(RustBridgeTest, ZeroLengthHandling) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 50);
    
    // Zero length should not detect motion
    EXPECT_FALSE(detector->detectMotion(frame1.data(), frame2.data(), 0, 10));
    
    auto result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 0, 10, 10);
    EXPECT_FALSE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 0);
    EXPECT_FLOAT_EQ(result.motion_level, 0.0f);
}

TEST_F(RustBridgeTest, DifferentThresholds) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 30);
    
    // Low threshold should detect motion
    EXPECT_TRUE(detector->detectMotion(frame1.data(), frame2.data(), 100, 10));
    
    // High threshold should not detect motion
    EXPECT_FALSE(detector->detectMotion(frame1.data(), frame2.data(), 100, 40));
}

TEST_F(RustBridgeTest, MotionLevelAccuracy) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 0);
    
    // Set exactly half pixels to maximum difference
    for (int i = 0; i < 50; ++i) {
        frame2[i] = 255;
    }
    
    auto result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 10, 10, 10);
    EXPECT_TRUE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 50);
    EXPECT_NEAR(result.motion_level, 1.0f, 0.01f);
}

TEST_F(RustBridgeTest, IdenticalFrames) {
    detector->initialize();
    
    std::vector<uint8_t> frame1(100, 128);
    std::vector<uint8_t> frame2(100, 128);
    
    EXPECT_FALSE(detector->detectMotion(frame1.data(), frame2.data(), 100, 10));
    
    auto result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 10, 10, 10);
    EXPECT_FALSE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, 0);
    EXPECT_FLOAT_EQ(result.motion_level, 0.0f);
}

TEST_F(RustBridgeTest, LargeFrameHandling) {
    detector->initialize();
    
    // Test with larger frame (HD resolution)
    const size_t hd_size = 1920 * 1080;
    std::vector<uint8_t> frame1(hd_size, 0);
    std::vector<uint8_t> frame2(hd_size, 0);
    
    // Add motion to enough pixels to exceed 5% threshold
    // 5% of 2,073,600 = 103,680 pixels
    const size_t changed_pixels = hd_size / 20 + 1000;  // Just over 5%
    for (size_t i = 0; i < changed_pixels; ++i) {
        frame2[i] = 100;
    }
    
    // Use a lower threshold for large frames
    EXPECT_TRUE(detector->detectMotion(frame1.data(), frame2.data(), hd_size, 5));
    
    auto result = detector->detectMotionAdvanced(frame1.data(), frame2.data(), 1920, 1080, 5);
    EXPECT_TRUE(result.motion_detected);
    EXPECT_EQ(result.changed_pixels, changed_pixels);
}

TEST_F(RustBridgeTest, MotionResultStructure) {
    // Test the MotionResult struct directly
    MotionResult result;
    result.motion_detected = true;
    result.motion_level = 0.75f;
    result.changed_pixels = 100;
    
    EXPECT_TRUE(result.motion_detected);
    EXPECT_FLOAT_EQ(result.motion_level, 0.75f);
    EXPECT_EQ(result.changed_pixels, 100);
}

// Test C interface functions directly
TEST(RustBridgeCTest, DirectCInterface) {
    // Test initialization
    EXPECT_TRUE(motion_init());
    
    std::vector<uint8_t> frame1(100, 0);
    std::vector<uint8_t> frame2(100, 50);
    
    // Test basic motion detection
    EXPECT_TRUE(detect_motion(frame1.data(), frame2.data(), 100, 10));
    
    // Test advanced motion detection
    auto result = detect_motion_advanced(frame1.data(), frame2.data(), 10, 10, 10);
    EXPECT_TRUE(result.motion_detected);
    EXPECT_GT(result.changed_pixels, 0);
    EXPECT_GT(result.motion_level, 0.0f);
    
    // Cleanup
    EXPECT_NO_THROW(motion_cleanup());
}
