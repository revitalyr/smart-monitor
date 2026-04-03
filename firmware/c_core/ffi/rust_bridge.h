/**
 * @file rust_bridge.h
 * @brief Foreign Function Interface (FFI) bridge between C and Rust.
 * 
 * This header declares the functions exported by the Rust motion detection module
 * and the C wrapper functions used to interact with them.
 */
#ifndef RUST_BRIDGE_H
#define RUST_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct motion_result_t
 * @brief Data structure returned by the motion detection algorithm.
 */
typedef struct {
    bool motion_detected;    /**< True if motion exceeds thresholds */
    float motion_level;      /**< Normalized motion level (0.0 - 1.0) */
    uint32_t changed_pixels; /**< Count of pixels identified as changed */
} motion_result_t;

/**
 * @struct rust_motion_detector_t
 * @brief Handle for the Rust-based motion detection engine.
 */
typedef struct {
    bool initialized;
} rust_motion_detector_t;

/* --- Rust FFI functions (Implemented in Rust) --- */

/**
 * @brief Basic motion detection on raw pixel buffers.
 * 
 * @param prev Previous frame buffer.
 * @param curr Current frame buffer.
 * @param len Length of the buffers.
 * @param threshold Difference threshold per pixel.
 * @return true if motion was detected.
 */
extern bool detect_motion(
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
);

/**
 * @brief Advanced motion detection with spatial awareness.
 * 
 * @param prev Previous frame buffer (Grayscale).
 * @param curr Current frame buffer (Grayscale).
 * @param width Frame width.
 * @param height Frame height.
 * @param threshold Pixel intensity difference threshold.
 * @return motion_result_t detailed results.
 */
extern motion_result_t detect_motion_advanced(
    const uint8_t* prev,
    const uint8_t* curr,
    uint32_t width,
    uint32_t height,
    uint8_t threshold
);

extern bool motion_init(void);
extern void motion_cleanup(void);

/* --- C Wrapper Functions --- */

/**
 * @brief Creates a new motion detector instance.
 * @return rust_motion_detector_t* Pointer to the new detector.
 */
rust_motion_detector_t* rust_detector_create(void);
void rust_detector_destroy(rust_motion_detector_t* detector);

/**
 * @brief Initializes the underlying Rust engine.
 */
bool rust_detector_initialize(rust_motion_detector_t* detector);
void rust_detector_cleanup(rust_motion_detector_t* detector);

/**
 * @brief C-friendly wrapper for basic motion detection.
 */
bool rust_detector_detect_motion(
    rust_motion_detector_t* detector,
    const uint8_t* prev,
    const uint8_t* curr,
    size_t len,
    uint8_t threshold
);

/**
 * @brief C-friendly wrapper for advanced motion detection.
 */
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
