#include "audio_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sound/asound.h>
#include <math.h>

#define ALSA_DEVICE "default"
#define BUFFER_SIZE 4096
#define VOICE_THRESHOLD 0.1f

audio_capture_t* audio_create(const char* device) {
    audio_capture_t* audio = malloc(sizeof(audio_capture_t));
    if (!audio) {
        return NULL;
    }
    
    memset(audio, 0, sizeof(audio_capture_t));
    audio->fd = -1;
    audio->sample_rate = 44100;
    audio->channels = 1;
    audio->buffer_size = BUFFER_SIZE;
    
    return audio;
}

audio_capture_t* audio_create_mock(void) {
    audio_capture_t* audio = audio_create("mock");
    if (audio) {
        audio->buffer = malloc(BUFFER_SIZE);
        audio->initialized = true;
    }
    return audio;
}

bool audio_initialize(audio_capture_t* audio, int sample_rate, int channels) {
    if (!audio) {
        return false;
    }
    
    audio->sample_rate = sample_rate;
    audio->channels = channels;
    
    // Try ALSA initialization
    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* hw_params;
    int err;
    
    if ((err = snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(err));
        return false;
    }
    
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        fprintf(stderr, "Cannot allocate hardware parameter structure: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return false;
    }
    
    if ((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot initialize hardware parameter structure: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return false;
    }
    
    // Set parameters
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
    
    if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
        fprintf(stderr, "Cannot set hardware parameters: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(hw_params);
        snd_pcm_close(pcm_handle);
        return false;
    }
    
    snd_pcm_hw_params_free(hw_params);
    
    audio->fd = 1; // Mark as initialized
    audio->initialized = true;
    
    return true;
}

void audio_destroy(audio_capture_t* audio) {
    if (!audio) {
        return;
    }
    
    if (audio->buffer) {
        free(audio->buffer);
    }
    
    free(audio);
}

bool audio_start_capture(audio_capture_t* audio) {
    if (!audio || !audio->initialized) {
        return false;
    }
    
    return true;
}

void audio_stop_capture(audio_capture_t* audio) {
    if (!audio) {
        return;
    }
}

int audio_read_samples(audio_capture_t* audio, uint8_t* buffer, int size) {
    if (!audio || !audio->initialized) {
        return -1;
    }
    
    // Mock implementation - generate simulated audio
    if (audio->fd == -1) {
        audio_generate_mock_samples(audio, buffer, size);
        return size;
    }
    
    // Real ALSA implementation would go here
    return size;
}

void audio_generate_mock_samples(audio_capture_t* audio, uint8_t* buffer, int size) {
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

audio_metrics_t audio_analyze_samples(const uint8_t* samples, int count) {
    audio_metrics_t metrics = {0};
    
    metrics.sample_count = count;
    metrics.noise_level = audio_calculate_rms(samples, count);
    metrics.voice_detected = audio_detect_voice_activity(samples, count, VOICE_THRESHOLD);
    
    // Calculate peak level
    int16_t* samples16 = (int16_t*)samples;
    float peak = 0.0f;
    for (int i = 0; i < count / 2; i++) {
        float abs_sample = fabsf((float)samples16[i]);
        if (abs_sample > peak) {
            peak = abs_sample;
        }
    }
    metrics.peak_level = peak / 32768.0f;
    
    return metrics;
}

float audio_calculate_rms(const uint8_t* samples, int count) {
    if (count == 0) return 0.0f;
    
    int16_t* samples16 = (int16_t*)samples;
    double sum = 0.0;
    
    for (int i = 0; i < count / 2; i++) {
        double sample = (double)samples16[i] / 32768.0;
        sum += sample * sample;
    }
    
    return sqrtf(sum / (count / 2));
}

bool audio_detect_voice_activity(const uint8_t* samples, int count, float threshold) {
    float rms = audio_calculate_rms(samples, count);
    return rms > threshold;
}
