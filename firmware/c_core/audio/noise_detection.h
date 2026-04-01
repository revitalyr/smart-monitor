#ifndef NOISE_DETECTION_H
#define NOISE_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float noise_level;
    float peak_frequency;
    bool baby_crying_detected;
    bool screaming_detected;
    float voice_activity;
    uint32_t sample_count;
    uint64_t timestamp;
} noise_metrics_t;

typedef struct {
    bool initialized;
    float crying_threshold;
    float screaming_threshold;
    float voice_threshold;
    int sample_rate;
    int window_size;
    float* fft_buffer;
    float* frequency_bins;
} noise_detector_t;

// Noise detection functions
noise_detector_t* noise_detector_create(int sample_rate);
bool noise_detector_initialize(noise_detector_t* detector);
void noise_detector_destroy(noise_detector_t* detector);
noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const uint8_t* samples, int count);

// Baby crying detection
bool detect_baby_crying(const uint8_t* samples, int count, int sample_rate);
bool detect_screaming(const uint8_t* samples, int count, int sample_rate);
float calculate_voice_activity(const uint8_t* samples, int count);

// Sleep score calculation
typedef struct {
    float motion_score;
    float noise_score;
    float overall_score;
    uint32_t sleep_quality_minutes;
    bool deep_sleep_detected;
} sleep_metrics_t;

sleep_metrics_t calculate_sleep_score(float motion_level, float noise_level, uint32_t duration_minutes);

#endif // NOISE_DETECTION_H
