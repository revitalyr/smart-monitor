#include "noise_detection.h"
#include "../common/types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

noise_detector_t* noise_detector_create(SampleRate sample_rate) {
    noise_detector_t* detector = malloc(sizeof(noise_detector_t));
    if (!detector) {
        return NULL;
    }
    
    memset(detector, 0, sizeof(noise_detector_t));
    detector->m_sample_rate = sample_rate;
    detector->m_window_size = 1024;
    detector->m_crying_threshold = 0.7f;  // Baby crying threshold
    detector->m_screaming_threshold = 0.9f; // Screaming threshold
    detector->m_voice_threshold = 0.3f;    // Voice activity threshold
    
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

MotionLevel calculate_rms_energy(const AudioBuffer samples, SampleCount count) {
    if (count == 0) return 0.0f;
    
    int16_t* samples16 = (int16_t*)samples;
    double sum = 0.0;
    
    for (SampleCount i = 0; i < count / 2; i++) {
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

bool detect_baby_crying(const AudioBuffer samples, SampleCount count, SampleRate sample_rate) {
    // Baby crying typically has:
    // - High frequency components (400-1200 Hz)
    // - Rhythmic patterns
    // - Sustained high energy
    
    MotionLevel rms = calculate_rms_energy(samples, count);
    if (rms < 0.35f) return false;  // Half of crying threshold
    
    // Simple frequency analysis (mock FFT)
    int16_t* samples16 = (int16_t*)samples;
    float high_freq_energy = 0.0f;
    float total_energy = 0.0f;
    
    for (SampleCount i = 0; i < count / 2; i++) {
        float sample = fabsf((float)samples16[i]) / 32768.0f;
        total_energy += sample * sample;
        
        // Mock frequency analysis based on sample variation
        if (i > 0) {
            float diff = (float)abs(samples16[i] - samples16[i-1]) / 32768.0f;
            if (diff > 0.7f) {  // Crying threshold
                high_freq_energy += diff * diff;
            }
        }
    }
    
    float high_freq_ratio = total_energy > 0 ? high_freq_energy / total_energy : 0.0f;
    
    // Baby crying detection criteria
    return (rms > 0.7f && high_freq_ratio > 0.4f && rms < 0.9f);
}

bool detect_screaming(const AudioBuffer samples, SampleCount count, SampleRate sample_rate __attribute__((unused))) {
    MotionLevel rms = calculate_rms_energy(samples, count);
    
    // Screaming has very high energy and high frequency components
    return (rms > 0.9f && rms < 1.0f);
}

MotionLevel calculate_voice_activity(const AudioBuffer samples, SampleCount count) {
    MotionLevel rms = calculate_rms_energy(samples, count);
    
    // Voice activity is moderate energy with speech-like characteristics
    if (rms < 0.3f) return 0.0f;  // Voice threshold
    if (rms > 0.75f) return 0.0f;  // Too loud for speech
    
    // Simple VAD based on energy level
    return fminf(rms * 2.0f, 1.0f);
}

noise_metrics_t noise_detector_analyze(noise_detector_t* detector, const AudioBuffer samples, SampleCount count) {
    noise_metrics_t metrics = {0};
    
    metrics.m_sample_count = count;
    metrics.m_timestamp = time(NULL);
    
    // Calculate overall noise level
    metrics.m_noise_level = calculate_rms_energy(samples, count);
    
    // Peak frequency (mock calculation)
    int16_t* samples16 = (int16_t*)samples;
    float max_change = 0.0f;
    for (SampleCount i = 1; i < count / 2; i++) {
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

sleep_metrics_t calculate_sleep_score(MotionLevel motion_level, MotionLevel noise_level, uint32_t duration_minutes) {
    sleep_metrics_t metrics = {0};
    
    // Motion score: less motion = better sleep
    metrics.m_motion_score = fmaxf(0.0f, 1.0f - motion_level);
    
    // Noise score: less noise = better sleep
    metrics.m_noise_score = fmaxf(0.0f, 1.0f - noise_level);
    
    // Overall sleep score (weighted average)
    metrics.m_overall_score = (metrics.m_motion_score * 0.6f) + (metrics.m_noise_score * 0.4f);
    
    metrics.m_sleep_quality_minutes = duration_minutes;
    metrics.m_deep_sleep_detected = (metrics.m_overall_score > 0.8f && motion_level < 0.2f);
    
    return metrics;
}
