#ifndef NOISE_DETECTION_H
#define NOISE_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/smart_monitor_constants.h"

typedef struct {
    noise_level_t m_noise_level;
    frequency_hz_t m_peak_frequency;
    bool m_baby_crying_detected;
    bool m_screaming_detected;
    noise_level_t m_voice_activity;
    sample_count_t m_sample_count;
    timestamp_t m_timestamp;
} noise_metrics_t;

typedef struct {
    bool m_initialized;
    noise_level_t m_crying_threshold;
    noise_level_t m_screaming_threshold;
    noise_level_t m_voice_threshold;
    sample_rate_t m_sample_rate;
    window_size_t m_window_size;
    fft_buffer_t m_fft_buffer;
    frequency_bins_t m_frequency_bins;
} noise_detector_t;

// Noise detection functions
noise_detector_t* noise_detector_create(sample_rate_t sample_rate);
bool noise_detector_initialize(noise_detector_t* detector);
void noise_detector_destroy(noise_detector_t* detector);
noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const uint8_t* samples, sample_count_t count);

// Baby crying detection
bool detect_baby_crying(const uint8_t* samples, sample_count_t count, sample_rate_t sample_rate);
bool detect_screaming(const uint8_t* samples, sample_count_t count, sample_rate_t sample_rate);
noise_level_t calculate_voice_activity(const uint8_t* samples, sample_count_t count);

// Sleep score calculation
typedef struct {
    motion_level_t m_motion_score;
    noise_level_t m_noise_score;
    sleep_score_t m_overall_score;
    sleep_duration_t m_sleep_quality_minutes;
    bool m_deep_sleep_detected;
} sleep_metrics_t;

sleep_metrics_t calculate_sleep_score(motion_level_t motion_level, noise_level_t noise_level, sleep_duration_t duration_minutes);

#endif // NOISE_DETECTION_H
