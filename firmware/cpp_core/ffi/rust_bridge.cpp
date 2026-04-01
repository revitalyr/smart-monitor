#include "rust_bridge.hpp"

RustMotionDetector::RustMotionDetector() : initialized_(false) {
}

RustMotionDetector::~RustMotionDetector() {
    if (initialized_) {
        cleanup();
    }
}

bool RustMotionDetector::initialize() {
    if (initialized_) {
        return true;
    }
    
    initialized_ = motion_init();
    return initialized_;
}

void RustMotionDetector::cleanup() {
    if (initialized_) {
        motion_cleanup();
        initialized_ = false;
    }
}

bool RustMotionDetector::detectMotion(
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
) {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }
    
    return detect_motion(prev, curr, len, threshold);
}

MotionResult RustMotionDetector::detectMotionAdvanced(
    const uint8_t* prev,
    const uint8_t* curr,
    uint32_t width,
    uint32_t height,
    uint8_t threshold
) {
    if (!initialized_) {
        if (!initialize()) {
            return MotionResult{false, 0.0f, 0};
        }
    }
    
    return detect_motion_advanced(prev, curr, width, height, threshold);
}
