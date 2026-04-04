#ifndef NOISE_DETECTION_H
#define NOISE_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/types.h"

typedef struct {
    MotionLevel m_noise_level;
    uint16_t m_peak_frequency;
    bool m_baby_crying_detected;
    bool m_screaming_detected;
    MotionLevel m_voice_activity;
    SampleCount m_sample_count;
    TimestampMs m_timestamp;
} noise_metrics_t;

typedef struct {
    bool m_initialized;
    MotionLevel m_crying_threshold;
    MotionLevel m_screaming_threshold;
    MotionLevel m_voice_threshold;
    SampleRate m_sample_rate;
    uint32_t m_window_size;
    void* m_fft_buffer;
    uint32_t m_frequency_bins;
} noise_detector_t;

// Noise detection functions
noise_detector_t* noise_detector_create(SampleRate sample_rate);
bool noise_detector_initialize(noise_detector_t* detector);
void noise_detector_destroy(noise_detector_t* detector);
noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const AudioBuffer samples, SampleCount count);

// Baby crying detection
bool detect_baby_crying(const AudioBuffer samples, SampleCount count, SampleRate sample_rate);
bool detect_screaming(const AudioBuffer samples, SampleCount count, SampleRate sample_rate);
MotionLevel calculate_voice_activity(const AudioBuffer samples, SampleCount count);

// Sleep score calculation
typedef struct {
    MotionLevel m_motion_score;
    MotionLevel m_noise_score;
    MotionLevel m_overall_score;
    uint32_t m_sleep_quality_minutes;
    bool m_deep_sleep_detected;
} sleep_metrics_t;

sleep_metrics_t calculate_sleep_score(MotionLevel motion_level, MotionLevel noise_level, uint32_t duration_minutes);

#endif // NOISE_DETECTION_H
