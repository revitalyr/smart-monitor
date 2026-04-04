/**
 * @file rust_bridge.h
 * @brief Foreign Function Interface (FFI) bridge between C and Rust.
 * 
 * This header declares the functions exported by the Rust motion detection module
 * and the C wrapper functions used to interact with them.
 */
#ifndef RUST_BRIDGE_H
#define RUST_BRIDGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct MotionResult
 * @brief Data structure returned by the motion detection algorithm.
 */
typedef struct {
    bool motion_detected;    /**< True if motion exceeds thresholds */
    MotionLevel motion_level; /**< Normalized motion level (0.0 - 1.0) */
    PixelCount changed_pixels; /**< Count of pixels identified as changed */
} MotionResult;

/**
 * @struct MotionDetector
 * @brief Handle for the Rust-based motion detection engine.
 */
typedef struct {
    bool initialized;
} MotionDetector;

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
    const FrameBuffer prev,
    const FrameBuffer curr,
    size_t len,
    MotionThreshold threshold
);

/**
 * @brief Advanced motion detection with spatial awareness.
 * 
 * @param prev Previous frame buffer (Grayscale).
 * @param curr Current frame buffer (Grayscale).
 * @param width Frame width.
 * @param height Frame height.
 * @param threshold Pixel intensity difference threshold.
 * @return MotionResult detailed results.
 */
extern MotionResult detect_motion_advanced(
    const FrameBuffer prev,
    const FrameBuffer curr,
    FrameWidth width,
    FrameHeight height,
    MotionThreshold threshold
);

extern bool motion_init(void);
extern void motion_cleanup(void);

/* --- C Wrapper Functions --- */

/**
 * @brief Creates a new motion detector instance.
 * @return MotionDetector* Pointer to the new detector.
 */
MotionDetector* rust_detector_create(void);
void rust_detector_destroy(MotionDetector* detector);

/**
 * @brief Initializes the underlying Rust engine.
 */
bool rust_detector_initialize(MotionDetector* detector);
void rust_detector_cleanup(MotionDetector* detector);

/**
 * @brief C-friendly wrapper for basic motion detection.
 */
bool rust_detector_detect_motion(
    MotionDetector* detector,
    const FrameBuffer prev,
    const FrameBuffer curr,
    size_t len,
    MotionThreshold threshold
);

/**
 * @brief C-friendly wrapper for advanced motion detection.
 */
MotionResult rust_detector_detect_motion_advanced(
    MotionDetector* detector,
    const FrameBuffer prev,
    const FrameBuffer curr,
    FrameWidth width,
    FrameHeight height,
    MotionThreshold threshold
);

#ifdef __cplusplus
}
#endif

#endif // RUST_BRIDGE_H
