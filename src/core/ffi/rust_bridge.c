#include "rust_bridge.h"
#include <string.h>
#include <stdlib.h>

// Mock implementations of Rust functions for testing without Rust compilation

MotionDetector* rust_detector_create(void) {
    MotionDetector* detector = malloc(sizeof(MotionDetector));
    if (detector) {
        memset(detector, 0, sizeof(MotionDetector));
        detector->initialized = true;
    }
    return detector;
}

bool rust_detector_initialize(MotionDetector* detector) {
    (void)detector;
    return true;
}

void rust_detector_destroy(MotionDetector* detector) {
    if (detector) {
        free(detector);
    }
}

bool rust_detector_detect_motion(MotionDetector* detector,
                                          const FrameBuffer prev,
                                          const FrameBuffer curr,
                                          size_t len,
                                          MotionThreshold threshold) {
    (void)detector; (void)prev; (void)curr; (void)len; (void)threshold;
    return false;
}

MotionResult rust_detector_detect_motion_advanced(MotionDetector* detector,
                                                 const FrameBuffer prev,
                                                 const FrameBuffer curr,
                                                 FrameWidth width,
                                                 FrameHeight height,
                                                 MotionThreshold threshold) {
    (void)detector; (void)prev; (void)curr; (void)width; (void)height; (void)threshold;
    
    MotionResult result;
    result.motion_detected = false;
    result.motion_level = 0.0f;
    result.changed_pixels = 0;
    return result;
}

// Legacy function names for compatibility
bool motion_init(void) {
    return true;
}

void motion_cleanup(void) {}

bool detect_motion(const FrameBuffer prev, const FrameBuffer curr, size_t len, MotionThreshold threshold) {
    (void)prev; (void)curr; (void)len; (void)threshold;
    return false;
}

MotionResult detect_motion_advanced(const FrameBuffer prev, const FrameBuffer curr, 
                                   FrameWidth width, FrameHeight height, MotionThreshold threshold) {
    (void)prev; (void)curr; (void)width; (void)height; (void)threshold;
    
    MotionResult result;
    result.motion_detected = false;
    result.motion_level = 0.0f;
    result.changed_pixels = 0;
    return result;
}
