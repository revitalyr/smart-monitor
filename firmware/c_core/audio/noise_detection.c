#include "noise_detection.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BABY_CRY_FREQ_MIN 400   // Hz
#define BABY_CRY_FREQ_MAX 1200  // Hz
#define SCREAM_FREQ_MIN 2000     // Hz
#define SCREAM_FREQ_MAX 4000     // Hz
#define VOICE_FREQ_MIN 100       // Hz
#define VOICE_FREQ_MAX 4000      // Hz

noise_detector_t* noise_detector_create(int sample_rate) {
    noise_detector_t* detector = malloc(sizeof(noise_detector_t));
    if (!detector) {
        return NULL;
    }
    
    memset(detector, 0, sizeof(noise_detector_t));
    detector->sample_rate = sample_rate;
    detector->window_size = 1024;
    detector->crying_threshold = 0.3f;
    detector->screaming_threshold = 0.5f;
    detector->voice_threshold = 0.1f;
    
    detector->fft_buffer = malloc(detector->window_size * sizeof(float));
    detector->frequency_bins = malloc(detector->window_size / 2 * sizeof(float));
    
    return detector;
}

bool noise_detector_initialize(noise_detector_t* detector) {
    if (!detector) {
        return false;
    }
    
    detector->initialized = true;
    return true;
}

void noise_detector_destroy(noise_detector_t* detector) {
    if (!detector) {
        return;
    }
    
    if (detector->fft_buffer) {
        free(detector->fft_buffer);
    }
    
    if (detector->frequency_bins) {
        free(detector->frequency_bins);
    }
    
    free(detector);
}

float calculate_rms_energy(const uint8_t* samples, int count) {
    if (count == 0) return 0.0f;
    
    int16_t* samples16 = (int16_t*)samples;
    double sum = 0.0;
    
    for (int i = 0; i < count / 2; i++) {
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

bool detect_baby_crying(const uint8_t* samples, int count, int sample_rate) {
    // Baby crying typically has:
    // - High frequency components (400-1200 Hz)
    // - Rhythmic patterns
    // - Sustained high energy
    
    float rms = calculate_rms_energy(samples, count);
    if (rms < 0.1f) return false;
    
    // Simple frequency analysis (mock FFT)
    int16_t* samples16 = (int16_t*)samples;
    float high_freq_energy = 0.0f;
    float total_energy = 0.0f;
    
    for (int i = 0; i < count / 2; i++) {
        float sample = fabsf((float)samples16[i]) / 32768.0f;
        total_energy += sample * sample;
        
        // Mock frequency analysis based on sample variation
        if (i > 0) {
            float diff = fabsf(samples16[i] - samples16[i-1]) / 32768.0f;
            if (diff > 0.1f) { // High frequency component
                high_freq_energy += diff * diff;
            }
        }
    }
    
    float high_freq_ratio = total_energy > 0 ? high_freq_energy / total_energy : 0.0f;
    
    // Baby crying detection criteria
    return (rms > 0.2f && high_freq_ratio > 0.3f && rms < 0.8f);
}

bool detect_screaming(const uint8_t* samples, int count, int sample_rate) {
    float rms = calculate_rms_energy(samples, count);
    
    // Screaming has very high energy and high frequency components
    return (rms > 0.5f && rms < 1.0f);
}

float calculate_voice_activity(const uint8_t* samples, int count) {
    float rms = calculate_rms_energy(samples, count);
    
    // Voice activity is moderate energy with speech-like characteristics
    if (rms < 0.05f) return 0.0f;
    if (rms > 0.8f) return 0.0f; // Too loud for normal speech
    
    // Simple VAD based on energy level
    return fminf(rms * 2.0f, 1.0f);
}

noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const uint8_t* samples, int count) {
    noise_metrics_t metrics = {0};
    
    metrics.sample_count = count;
    metrics.timestamp = time(NULL);
    
    // Calculate overall noise level
    metrics.noise_level = calculate_rms_energy(samples, count);
    
    // Peak frequency (mock calculation)
    int16_t* samples16 = (int16_t*)samples;
    float max_change = 0.0f;
    for (int i = 1; i < count / 2; i++) {
        float change = fabsf(samples16[i] - samples16[i-1]);
        if (change > max_change) {
            max_change = change;
        }
    }
    metrics.peak_frequency = max_change * 100.0f; // Mock frequency
    
    // Detect specific sound types
    metrics.baby_crying_detected = detect_baby_crying(samples, count, detector->sample_rate);
    metrics.screaming_detected = detect_screaming(samples, count, detector->sample_rate);
    metrics.voice_activity = calculate_voice_activity(samples, count);
    
    return metrics;
}

sleep_metrics_t calculate_sleep_score(float motion_level, float noise_level, uint32_t duration_minutes) {
    sleep_metrics_t metrics = {0};
    
    // Motion score: less motion = better sleep
    metrics.motion_score = fmaxf(0.0f, 1.0f - motion_level);
    
    // Noise score: less noise = better sleep
    metrics.noise_score = fmaxf(0.0f, 1.0f - noise_level);
    
    // Overall sleep score (weighted average)
    metrics.overall_score = (metrics.motion_score * 0.6f) + (metrics.noise_score * 0.4f);
    
    // Deep sleep detected if both motion and noise are very low
    metrics.deep_sleep_detected = (motion_level < 0.1f && noise_level < 0.1f);
    
    // Sleep quality minutes based on overall score
    metrics.sleep_quality_minutes = (uint32_t)(duration_minutes * metrics.overall_score);
    
    return metrics;
}
