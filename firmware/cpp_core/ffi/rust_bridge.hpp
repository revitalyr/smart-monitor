#pragma once
#include <cstddef>
#include <cstdint>

/// Motion detection result from Rust
struct MotionResult {
    bool motion_detected;
    float motion_level;
    uint32_t changed_pixels;
};

extern "C" {
    /// Basic motion detection
    bool detect_motion(
        const uint8_t* prev,
        const uint8_t* curr,
        size_t len,
        uint8_t threshold
    );

    /// Advanced motion detection with detailed metrics
    MotionResult detect_motion_advanced(
        const uint8_t* prev,
        const uint8_t* curr,
        uint32_t width,
        uint32_t height,
        uint8_t threshold
    );

    /// Initialize motion detection engine
    bool motion_init();

    /// Cleanup motion detection engine
    void motion_cleanup();
}

/// C++ wrapper class for safe Rust interaction
class RustMotionDetector {
public:
    RustMotionDetector();
    ~RustMotionDetector();
    
    bool initialize();
    void cleanup();
    
    bool detectMotion(
        const uint8_t* prev,
        const uint8_t* curr,
        size_t len,
        uint8_t threshold = 20
    );
    
    MotionResult detectMotionAdvanced(
        const uint8_t* prev,
        const uint8_t* curr,
        uint32_t width,
        uint32_t height,
        uint8_t threshold = 20
    );

private:
    bool initialized_;
};
