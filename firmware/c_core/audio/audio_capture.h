#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int fd;
    bool initialized;
    bool capturing;
    int sample_rate;
    int channels;
    int buffer_size;
    uint8_t* buffer;
} audio_capture_t;

typedef struct {
    float noise_level;
    float peak_level;
    bool voice_detected;
    uint32_t sample_count;
} audio_metrics_t;

// Audio capture functions
audio_capture_t* audio_create(const char* device);
bool audio_initialize(audio_capture_t* audio, int sample_rate, int channels);
void audio_destroy(audio_capture_t* audio);
bool audio_start_capture(audio_capture_t* audio);
void audio_stop_capture(audio_capture_t* audio);
int audio_read_samples(audio_capture_t* audio, uint8_t* buffer, int size);

// Audio analysis functions
audio_metrics_t audio_analyze_samples(const uint8_t* samples, int count);
float audio_calculate_rms(const uint8_t* samples, int count);
bool audio_detect_voice_activity(const uint8_t* samples, int count, float threshold);

// Mock audio functions
audio_capture_t* audio_create_mock(void);
void audio_generate_mock_samples(audio_capture_t* audio, uint8_t* buffer, int size);

#endif // AUDIO_CAPTURE_H
