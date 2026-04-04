#include <stdbool.h>
#include "audio_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

// Mock ALSA implementation for demonstration
// In real deployment, this would use actual ALSA headers

#define ALSA_DEVICE "default"
#define BUFFER_SIZE 4096
#define VOICE_THRESHOLD 0.1f

AudioCapture* audio_create(const char* device) {
    (void)device; // Suppress unused parameter warning
    AudioCapture* audio = malloc(sizeof(AudioCapture));
    if (!audio) {
        return NULL;
    }
    
    memset(audio, 0, sizeof(AudioCapture));
    audio->fd = -1;
    audio->sample_rate = DEFAULT_AUDIO_SAMPLE_RATE;
    audio->channels = 1;
    audio->capturing = false;
    audio->buffer_size = BUFFER_SIZE;
    
    return audio;
}

AudioCapture* audio_create_mock(void) {
    AudioCapture* audio = audio_create("mock");
    if (audio) {
        audio->buffer = malloc(BUFFER_SIZE);
        audio->initialized = true;
    }
    return audio;
}

bool audio_initialize(AudioCapture* audio, SampleRate sample_rate, uint8_t channels) {
    if (!audio) {
        return false;
    }
    
    audio->sample_rate = sample_rate;
    audio->channels = channels;
    
    // Mock ALSA initialization - always succeed for demo
    audio->fd = 1; // Mark as initialized
    audio->initialized = true;
    
    return true;
}

void audio_destroy(AudioCapture* audio) {
    if (!audio) {
        return;
    }
    
    if (audio->buffer) {
        free(audio->buffer);
    }
    
    free(audio);
}

bool audio_start_capture(AudioCapture* audio) {
    if (!audio || !audio->initialized) {
        return false;
    }
    
    audio->capturing = true;
    return true;
}

void audio_stop_capture(AudioCapture* audio) {
    if (!audio) {
        return;
    }
    audio->capturing = false;
}

bool audio_is_capturing(const AudioCapture* audio) {
    if (!audio) {
        return false;
    }
    return audio->capturing;
}

int audio_read_samples(AudioCapture* audio, AudioBuffer buffer, ByteCount size) {
    if (!audio || !audio->initialized) {
        return -1;
    }
    
    // Mock implementation - generate simulated audio
    if (audio->fd == -1) {
        audio_generate_mock_samples(audio, buffer, size);
    }
    
    return size;
}

void audio_generate_mock_samples(AudioCapture* audio, AudioBuffer buffer, ByteCount size) {
    (void)audio; // Suppress unused parameter warning
    static float phase = 0.0f;
    static uint32_t counter = 0;
    
    for (int i = 0; i < size; i += 2) {
        // Generate simulated audio with occasional "voice" activity
        float sample = 0.0f;
        
        // Add periodic "voice" simulation
        if ((counter / 1000) % 10 == 0) {
            sample += sin(phase) * 0.3f; // Voice-like frequency
            phase += 0.1f;
        }
        
        // Add background noise
        sample += ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        
        // Convert to 16-bit PCM
        int16_t sample16 = (int16_t)(sample * 32767.0f);
        buffer[i] = sample16 & 0xFF;
        if (i + 1 < size) {
            buffer[i + 1] = (sample16 >> 8) & 0xFF;
        }
        
        counter++;
    }
}

AudioMetrics audio_analyze_samples(const AudioBuffer samples, SampleCount count) {
    AudioMetrics metrics = {0};
    
    metrics.sample_count = count;
    metrics.noise_level = audio_calculate_rms(samples, count);
    metrics.voice_detected = audio_detect_voice_activity(samples, count, VOICE_THRESHOLD);
    
    // Calculate peak level
    int16_t* samples16 = (int16_t*)samples;
    MotionLevel peak = 0.0f;
    for (int i = 0; i < count / 2; i++) {
        MotionLevel abs_sample = fabsf((MotionLevel)samples16[i]);
        if (abs_sample > peak) {
            peak = abs_sample;
        }
    }
    metrics.peak_level = peak / 32768.0f;
    
    return metrics;
}

MotionLevel audio_calculate_rms(const AudioBuffer samples, SampleCount count) {
    if (count == 0) return 0.0f;
    
    int16_t* samples16 = (int16_t*)samples;
    double sum = 0.0;
    
    for (int i = 0; i < count / 2; i++) {
        double sample = (double)samples16[i] / 32768.0;
        sum += sample * sample;
    }
    
    return sqrtf(sum / (count / 2));
}

bool audio_detect_voice_activity(const AudioBuffer samples, SampleCount count, MotionLevel threshold) {
    MotionLevel rms = audio_calculate_rms(samples, count);
    return rms > threshold;
}
