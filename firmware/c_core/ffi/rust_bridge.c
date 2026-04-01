#include "rust_bridge.h"
#include <stdlib.h>
#include <string.h>

rust_motion_detector_t* rust_detector_create(void) {
    rust_motion_detector_t* detector = malloc(sizeof(rust_motion_detector_t));
    if (detector) {
        memset(detector, 0, sizeof(rust_motion_detector_t));
    }
    return detector;
}

void rust_detector_destroy(rust_motion_detector_t* detector) {
    if (detector) {
        rust_detector_cleanup(detector);
        free(detector);
    }
}

bool rust_detector_initialize(rust_motion_detector_t* detector) {
    if (!detector) {
        return false;
    }
    
    if (detector->initialized) {
        return true;
    }
    
    detector->initialized = motion_init();
    return detector->initialized;
}

void rust_detector_cleanup(rust_motion_detector_t* detector) {
    if (detector && detector->initialized) {
        motion_cleanup();
        detector->initialized = false;
    }
}

bool rust_detector_detect_motion(
    rust_motion_detector_t* detector,
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
) {
    if (!detector) {
        return false;
    }
    
    if (!detector->initialized) {
        if (!rust_detector_initialize(detector)) {
            return false;
        }
    }
    
    return detect_motion(prev, curr, len, threshold);
}

motion_result_t rust_detector_detect_motion_advanced(
    rust_motion_detector_t* detector,
    const uint8_t* prev,
    const uint8_t* curr,
    uint32_t width,
    uint32_t height,
    uint8_t threshold
) {
    motion_result_t result = {false, 0.0f, 0};
    
    if (!detector) {
        return result;
    }
    
    if (!detector->initialized) {
        if (!rust_detector_initialize(detector)) {
            return result;
        }
    }
    
    return detect_motion_advanced(prev, curr, width, height, threshold);
}
