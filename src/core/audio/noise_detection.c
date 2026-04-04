#include "noise_detection.h"
#include "../common/smart_monitor_constants.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

noise_detector_t* noise_detector_create(sample_rate_t sample_rate) {
    noise_detector_t* detector = malloc(sizeof(noise_detector_t));
    if (!detector) {
        return NULL;
    }
    
    memset(detector, 0, sizeof(noise_detector_t));
    detector->m_sample_rate = sample_rate;
    detector->m_window_size = 1024;
    detector->m_crying_threshold = AUDIO_BABY_CRYING_THRESHOLD;
    detector->m_screaming_threshold = AUDIO_SCREAMING_THRESHOLD;
    detector->m_voice_threshold = AUDIO_VOICE_ACTIVITY_THRESHOLD;
    
    detector->m_fft_buffer = malloc(detector->m_window_size * sizeof(float));
    detector->m_frequency_bins = malloc(detector->m_window_size / 2 * sizeof(float));
    
    return detector;
}

bool noise_detector_initialize(noise_detector_t* detector) {
    if (!detector) {
        return false;
    }
    
    detector->m_initialized = true;
    return true;
}

void noise_detector_destroy(noise_detector_t* detector) {
    if (!detector) {
        return;
    }
    
    if (detector->m_fft_buffer) {
        free(detector->m_fft_buffer);
    }
    
    if (detector->m_frequency_bins) {
        free(detector->m_frequency_bins);
    }
    
    free(detector);
}

float calculate_rms_energy(const uint8_t* samples, sample_count_t count) {
    if (count == 0) return 0.0f;
    
    int16_t* samples16 = (int16_t*)samples;
    double sum = 0.0;
    
    for (sample_count_t i = 0; i < count / 2; i++) {
        double sample = (double)samples16[i] / 32768.0;
        sum += sample * sample;
    }
    
    return sqrtf(sum / (count / 2));
}

float calculate_frequency_energy(const float* frequency_bins, int bin_count, 
                               int sample_rate, float min_freq, float max_freq) {
    float energy = 0.0f;
    int min_bin = (int)(min_freq * bin_count * 2 / sample_rate);
    int max_bin = (int)(max_freq * bin_count * 2 / sample_rate);
    
    for (int i = min_bin; i < max_bin && i < bin_count; i++) {
        energy += frequency_bins[i] * frequency_bins[i];
    }
    
    return energy;
}

bool detect_baby_crying(const uint8_t* samples, sample_count_t count, sample_rate_t sample_rate __attribute__((unused))) {
    // Baby crying typically has:
    // - High frequency components (400-1200 Hz)
    // - Rhythmic patterns
    // - Sustained high energy
    
    noise_level_t rms = calculate_rms_energy(samples, count);
    if (rms < AUDIO_BABY_CRYING_THRESHOLD * 0.5f) return false;
    
    // Simple frequency analysis (mock FFT)
    int16_t* samples16 = (int16_t*)samples;
    float high_freq_energy = 0.0f;
    float total_energy = 0.0f;
    
    for (sample_count_t i = 0; i < count / 2; i++) {
        float sample = fabsf((float)samples16[i]) / 32768.0f;
        total_energy += sample * sample;
        
        // Mock frequency analysis based on sample variation
        if (i > 0) {
            float diff = (float)abs(samples16[i] - samples16[i-1]) / 32768.0f;
            if (diff > AUDIO_BABY_CRYING_THRESHOLD) {
                high_freq_energy += diff * diff;
            }
        }
    }
    
    float high_freq_ratio = total_energy > 0 ? high_freq_energy / total_energy : 0.0f;
    
    // Baby crying detection criteria
    return (rms > AUDIO_BABY_CRYING_THRESHOLD && high_freq_ratio > 0.4f && rms < AUDIO_SCREAMING_THRESHOLD);
}

bool detect_screaming(const uint8_t* samples, sample_count_t count, sample_rate_t sample_rate __attribute__((unused))) {
    noise_level_t rms = calculate_rms_energy(samples, count);
    
    // Screaming has very high energy and high frequency components
    return (rms > AUDIO_SCREAMING_THRESHOLD && rms < AUDIO_NOISE_LEVEL_MAX);
}

noise_level_t calculate_voice_activity(const uint8_t* samples, sample_count_t count) {
    noise_level_t rms = calculate_rms_energy(samples, count);
    
    // Voice activity is moderate energy with speech-like characteristics
    if (rms < AUDIO_VOICE_ACTIVITY_THRESHOLD) return AUDIO_NOISE_LEVEL_MIN;
    if (rms > AUDIO_VOICE_ACTIVITY_THRESHOLD * 2.5f) return AUDIO_NOISE_LEVEL_MIN;
    
    // Simple VAD based on energy level
    return fminf(rms * 2.0f, AUDIO_NOISE_LEVEL_MAX);
}

noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const uint8_t* samples, sample_count_t count) {
    noise_metrics_t metrics = {0};
    
    metrics.m_sample_count = count;
    metrics.m_timestamp = time(NULL);
    
    // Calculate overall noise level
    metrics.m_noise_level = calculate_rms_energy(samples, count);
    
    // Peak frequency (mock calculation)
    int16_t* samples16 = (int16_t*)samples;
    float max_change = 0.0f;
    for (sample_count_t i = 1; i < count / 2; i++) {
        float change = (float)abs(samples16[i] - samples16[i-1]);
        if (change > max_change) {
            max_change = change;
        }
    }
    metrics.m_peak_frequency = max_change * 100.0f; // Mock frequency
    
    // Detect specific sound types
    metrics.m_baby_crying_detected = detect_baby_crying(samples, count, detector->m_sample_rate);
    metrics.m_screaming_detected = detect_screaming(samples, count, detector->m_sample_rate);
    metrics.m_voice_activity = calculate_voice_activity(samples, count);
    
    return metrics;
}

sleep_metrics_t calculate_sleep_score(motion_level_t motion_level, noise_level_t noise_level, sleep_duration_t duration_minutes) {
    sleep_metrics_t metrics = {0};
    
    // Motion score: less motion = better sleep
    metrics.m_motion_score = fmaxf(0.0f, 1.0f - motion_level);
    
    // Noise score: less noise = better sleep
    metrics.m_noise_score = fmaxf(0.0f, 1.0f - noise_level);
    
    // Overall sleep score (weighted average)
    metrics.m_overall_score = (metrics.m_motion_score * SLEEP_MOTION_WEIGHT) + (metrics.m_noise_score * SLEEP_NOISE_WEIGHT);
    
    // Deep sleep detected if both motion and noise are very low
    metrics.m_deep_sleep_detected = (motion_level < SLEEP_DEEP_THRESHOLD && noise_level < SLEEP_DEEP_THRESHOLD);
    
    // Sleep quality minutes based on overall score
    metrics.m_sleep_quality_minutes = (uint32_t)(duration_minutes * metrics.m_overall_score);
    
    return metrics;
}
