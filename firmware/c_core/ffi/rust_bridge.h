#ifndef RUST_BRIDGE_H
#define RUST_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool motion_detected;
    float motion_level;
    uint32_t changed_pixels;
} motion_result_t;

typedef struct {
    bool initialized;
} rust_motion_detector_t;

// Rust FFI functions
extern bool detect_motion(
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
);

extern motion_result_t detect_motion_advanced(
    const uint8_t* prev,
    const uint8_t* curr,
    uint32_t width,
    uint32_t height,
    uint8_t threshold
);

extern bool motion_init(void);
extern void motion_cleanup(void);

// C wrapper functions
rust_motion_detector_t* rust_detector_create(void);
void rust_detector_destroy(rust_motion_detector_t* detector);

bool rust_detector_initialize(rust_motion_detector_t* detector);
void rust_detector_cleanup(rust_motion_detector_t* detector);

bool rust_detector_detect_motion(
    rust_motion_detector_t* detector,
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
);

motion_result_t rust_detector_detect_motion_advanced(
    rust_motion_detector_t* detector,
    const uint8_t* prev,
    const uint8_t* curr,
    uint32_t width,
    uint32_t height,
    uint8_t threshold
);

#ifdef __cplusplus
}
#endif

#endif // RUST_BRIDGE_H
