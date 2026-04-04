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

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Audio capture device structure
 * 
 * Contains device state, configuration, and buffer information
 * for audio capture operations.
 */
typedef struct {
    int fd;                 /**< Device file descriptor */
    bool initialized;        /**< Initialization status flag */
    bool capturing;          /**< Active capture status */
    int sample_rate;         /**< Audio sample rate in Hz */
    int channels;            /**< Number of audio channels */
    int buffer_size;         /**< Size of audio buffer in bytes */
    uint8_t* buffer;        /**< Pointer to audio buffer */
} audio_capture_t;

/**
 * @brief Audio metrics structure
 * 
 * Contains real-time audio analysis metrics including noise levels,
 * peak detection, and voice activity detection results.
 */
typedef struct {
    float noise_level;       /**< Current noise level (0.0-1.0) */
    float peak_level;        /**< Peak audio level (0.0-1.0) */
    bool voice_detected;      /**< Voice activity detection flag */
    uint32_t sample_count;   /**< Total samples processed */
} audio_metrics_t;

/**
 * @brief Create audio capture device instance
 * 
 * @param device Path to audio device (e.g., "/dev/dsp0")
 * @return Pointer to audio_capture_t instance or NULL on failure
 */
audio_capture_t* audio_create(const char* device);

/**
 * @brief Initialize audio capture device
 * 
 * @param audio Pointer to audio_capture_t instance
 * @param sample_rate Audio sample rate in Hz
 * @param channels Number of audio channels (1=mono, 2=stereo)
 * @return true on success, false on failure
 */
bool audio_initialize(audio_capture_t* audio, int sample_rate, int channels);

/**
 * @brief Destroy audio capture device and free resources
 * 
 * @param audio Pointer to audio_capture_t instance
 */
void audio_destroy(audio_capture_t* audio);

/**
 * @brief Start audio capture
 * 
 * @param audio Pointer to audio_capture_t instance
 * @return true on success, false on failure
 */
bool audio_start_capture(audio_capture_t* audio);

/**
 * @brief Stop audio capture
 * 
 * @param audio Pointer to audio_capture_t instance
 */
void audio_stop_capture(audio_capture_t* audio);

/**
 * @brief Read audio samples from device
 * 
 * @param audio Pointer to audio_capture_t instance
 * @param buffer Buffer to store audio samples
 * @param size Size of buffer in bytes
 * @return Number of bytes read, or -1 on error
 */
int audio_read_samples(audio_capture_t* audio, uint8_t* buffer, int size);

/**
 * @brief Analyze audio samples for metrics
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples to analyze
 * @return Audio metrics structure with analysis results
 */
audio_metrics_t audio_analyze_samples(const uint8_t* samples, int count);

/**
 * @brief Calculate RMS (Root Mean Square) of audio samples
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples
 * @return RMS value (0.0-1.0)
 */
float audio_calculate_rms(const uint8_t* samples, int count);

/**
 * @brief Detect voice activity in audio samples
 * 
 * @param samples Pointer to audio sample data
 * @param count Number of samples
 * @param threshold Detection threshold (0.0-1.0)
 * @return true if voice activity detected, false otherwise
 */
bool audio_detect_voice_activity(const uint8_t* samples, int count, float threshold);

/**
 * @brief Create mock audio capture device for testing
 * 
 * @return Pointer to mock audio_capture_t instance
 */
audio_capture_t* audio_create_mock(void);

/**
 * @brief Generate mock audio samples for testing
 * 
 * @param audio Pointer to audio_capture_t instance
 * @param buffer Buffer to store generated samples
 * @param size Size of buffer in bytes
 */
void audio_generate_mock_samples(audio_capture_t* audio, uint8_t* buffer, int size);

#endif // AUDIO_CAPTURE_H
