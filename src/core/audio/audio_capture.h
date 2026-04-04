/**
 * @file audio_capture.h
 * @brief Audio capture interface for Smart Monitor system
 * 
 * This module provides audio capture functionality including device initialization,
 * audio stream management, and real-time audio metrics collection.
 * Supports various audio input devices and provides callbacks for audio processing.
 */

#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "../common/types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Audio capture device structure
 * 
 * Contains device state, configuration, and buffer information
 * for audio capture operations.
 */
typedef struct {
    FileDescriptor fd;                 /**< Device file descriptor */
    bool initialized;                   /**< Initialization status flag */
    bool capturing;                     /**< Active capture status */
    SampleRate sample_rate;             /**< Audio sample rate in Hz */
    uint8_t channels;                   /**< Number of audio channels */
    ByteCount buffer_size;              /**< Size of audio buffer in bytes */
    AudioBuffer buffer;                 /**< Pointer to audio buffer */
} AudioCapture;

/**
 * @brief Audio metrics structure
 * 
 * Contains real-time audio analysis metrics including noise levels,
 * peak detection, and voice activity detection results.
 */
typedef struct {
    MotionLevel noise_level;       /**< Current noise level (0.0-1.0) */
    MotionLevel peak_level;        /**< Peak audio level (0.0-1.0) */
    bool voice_detected;            /**< Voice activity detection flag */
    SampleCount sample_count;       /**< Total samples processed */
} AudioMetrics;

/**
 * @brief Create audio capture device instance
 * 
 * @param device Path to audio device (e.g., "/dev/dsp0")
 * @return Pointer to AudioCapture instance or NULL on failure
 */
AudioCapture* audio_create(const char* device);

/**
 * @brief Initialize audio capture device
 * 
 * @param audio Pointer to AudioCapture instance
 * @param sample_rate Audio sample rate in Hz
 * @param channels Number of audio channels (1=mono, 2=stereo)
 * @return true on success, false on failure
 */
bool audio_initialize(AudioCapture* audio, SampleRate sample_rate, uint8_t channels);

/**
 * @brief Destroy audio capture device and free resources
 * 
 * @param audio Pointer to AudioCapture instance
 */
void audio_destroy(AudioCapture* audio);

/**
 * @brief Start audio capture
 * 
 * @param audio Pointer to AudioCapture instance
 * @return true on success, false on failure
 */
bool audio_start_capture(AudioCapture* audio);

/**
 * @brief Stop audio capture
 * 
 * @param audio Pointer to AudioCapture instance
 */
void audio_stop_capture(AudioCapture* audio);

/**
 * @brief Read audio samples from device
 * 
 * @param audio Pointer to AudioCapture instance
 * @param buffer Buffer to store audio samples
 * @param size Size of buffer in bytes
 * @return Number of bytes read, or -1 on error
 */
int audio_read_samples(AudioCapture* audio, AudioBuffer buffer, ByteCount size);

/**
 * @brief Analyze audio samples for metrics
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples to analyze
 * @return Audio metrics structure with analysis results
 */
AudioMetrics audio_analyze_samples(const AudioBuffer samples, SampleCount count);

/**
 * @brief Calculate RMS (Root Mean Square) of audio samples
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples
 * @return RMS value (0.0-1.0)
 */
MotionLevel audio_calculate_rms(const AudioBuffer samples, SampleCount count);

/**
 * @brief Detect voice activity in audio samples
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples
 * @param threshold Detection threshold (0.0-1.0)
 * @return true if voice activity detected, false otherwise
 */
bool audio_detect_voice_activity(const AudioBuffer samples, SampleCount count, MotionLevel threshold);

/**
 * @brief Create mock audio capture device for testing
 * 
 * @return Pointer to mock AudioCapture instance
 */
AudioCapture* audio_create_mock(void);

/**
 * @brief Generate mock audio samples for testing
 * 
 * @param audio Pointer to AudioCapture instance
 * @param buffer Buffer to store generated samples
 * @param size Size of buffer in bytes
 */
void audio_generate_mock_samples(AudioCapture* audio, AudioBuffer buffer, ByteCount size);

#endif // AUDIO_CAPTURE_H
