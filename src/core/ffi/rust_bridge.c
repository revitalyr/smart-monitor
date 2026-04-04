#include "rust_bridge.h"
#include <string.h>
#include <stdlib.h>

// Mock implementations of Rust functions for testing without Rust compilation

rust_motion_detector_t* rust_detector_create(void) {
    rust_motion_detector_t* detector = malloc(sizeof(rust_motion_detector_t));
    if (detector) {
        memset(detector, 0, sizeof(rust_motion_detector_t));
        detector->initialized = true;
    }
    return detector;
}

bool rust_detector_initialize(rust_motion_detector_t* detector) {
    (void)detector;
    return true;
}

void rust_detector_destroy(rust_motion_detector_t* detector) {
    if (detector) {
        free(detector);
    }
}

bool rust_detector_detect_motion(rust_motion_detector_t* detector,
                                          const uint8_t* prev,
                                          const uint8_t* curr,
                                          size_t len,
                                          uint8_t threshold) {
    (void)detector; (void)prev; (void)curr; (void)len; (void)threshold;
    return false;
}

motion_result_t rust_detector_detect_motion_advanced(rust_motion_detector_t* detector,
                                                 const uint8_t* prev,
                                                 const uint8_t* curr,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 uint8_t threshold) {
    (void)detector; (void)prev; (void)curr; (void)width; (void)height; (void)threshold;
    
    motion_result_t result;
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

bool detect_motion(const uint8_t* prev, const uint8_t* curr, size_t len, uint8_t threshold) {
    (void)prev; (void)curr; (void)len; (void)threshold;
    return false;
}

motion_result_t detect_motion_advanced(const uint8_t* prev, const uint8_t* curr, 
                                   uint32_t width, uint32_t height, uint8_t threshold) {
    (void)prev; (void)curr; (void)width; (void)height; (void)threshold;
    
    motion_result_t result;
    result.motion_detected = false;
    result.motion_level = 0.0f;
    result.changed_pixels = 0;
    return result;
}
