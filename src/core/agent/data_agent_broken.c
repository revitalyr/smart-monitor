#include "data_agent.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

// Global simulation state
static baby_simulation_t g_sim = {
    .state = BABY_SLEEPING_PEACEFUL,
    .room_temp = 22.0f,
    .body_temp = 36.6f,
    .humidity = 45.0f,
    .light_lux = 3,      // almost dark - nightlight off
    .target_noise = 0.02f,
    .smooth_noise = 0.02f,
    .smooth_motion_prob = 0.0f,
    .event_count = 0,
    .last_trigger = TRIGGER_RANDOM,
    .nightlight_on = false,
    .nightlight_timer = 0
};

static void update_baby_state(uint32_t delta_ms);
static void generate_realistic_sensor_data(SensorData* data);
static void generate_realistic_audio_data(AudioData* data);

const char* get_state_name(baby_state_t state) {
    switch (state) {
        case BABY_SLEEPING_PEACEFUL: return "SLEEPING_PEACEFUL";
        case BABY_RESTLESS: return "RESTLESS";
        case BABY_STIRRING: return "STIRRING";
        case BABY_FUSSY: return "FUSSY";
        case BABY_CRYING_SOFT: return "CRYING_SOFT";
        case BABY_CRYING_LOUD: return "CRYING_LOUD";
        case BABY_SCREAMING: return "SCREAMING";
        case BABY_FALLING_ASLEEP: return "FALLING_ASLEEP";
        default: return "UNKNOWN";
    }
}

// Helper function to get current timestamp
static uint32_t get_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// ---------------------------------------------------------------------------
// Helper LP-filter (first order RC-analog)
//   alpha = dt / (tau + dt), tau - smoothing time constant
// ---------------------------------------------------------------------------
static inline float lp_filter(float current, float target, float alpha) {
    return current + alpha * (target - current);
}

// Small Gaussian noise via CLT (sum of uniform)
static float gauss_noise(float sigma) {
    float s = 0.0f;
    for (int i = 0; i < 6; i++) s += (float)(rand() % 1000) / 1000.0f;
    return (s - 3.0f) * sigma;  // zero mean, STD ≈ sigma
}

// ---------------------------------------------------------------------------
// Physical parameters by state
//   Returns "target" values; real ones slowly converge to them.
// ---------------------------------------------------------------------------
typedef struct {
    float body_temp_target;     // °C
    float humidity_delta;       // addition to base humidity, %
    float noise_target;         // [0..1]
    float motion_prob;          // motion probability [0..1]
    bool  cry;
    bool  scream;
} state_physics_t;

static state_physics_t get_state_physics(baby_state_t s) {
    switch (s) {
        case BABY_SLEEPING_PEACEFUL: return (state_physics_t){36.6f, 0.0f, 0.02f, 0.00f, false, false};
        case BABY_RESTLESS:          return (state_physics_t){36.7f, 0.5f, 0.08f, 0.15f, false, false};
        case BABY_STIRRING:          return (state_physics_t){36.8f, 1.0f, 0.16f, 0.35f, false, false};
        case BABY_FUSSY:             return (state_physics_t){36.9f, 1.5f, 0.28f, 0.55f, false, false};
        case BABY_CRYING_SOFT:        return (state_physics_t){37.1f, 2.0f, 0.45f, 0.75f, true, false};
        case BABY_CRYING_LOUD:        return (state_physics_t){37.2f, 2.5f, 0.65f, 0.85f, true, false};
        case BABY_SCREAMING:          return (state_physics_t){37.3f, 3.0f, 0.85f, 0.95f, false, true};
        case BABY_FALLING_ASLEEP:      return (state_physics_t){36.8f, 1.0f, 0.12f, 0.05f, false, false};
        default:                        return (state_physics_t){36.6f, 0.0f, 0.02f, 0.00f, false, false};
    }
}

// ---------------------------------------------------------------------------
// Update room and body physics (called every delta_ms)
// ---------------------------------------------------------------------------
static void update_room_physics(uint32_t delta_ms) {
    float dt = (float)delta_ms / 1000.0f;  // seconds

    // Air temperature: slow daily drift + weak noise
    // Sometimes (once in ~5 min) room starts gradually cooling or warming
    static float room_temp_drift = 0.0f;
    static uint32_t drift_timer = 0;
    drift_timer += delta_ms;
    if (drift_timer > 300000) {  // every 5 minutes change drift
        drift_timer = 0;
        room_temp_drift = gauss_noise(0.003f);  // °C/s
    }
    g_sim.room_temp += room_temp_drift * dt + gauss_noise(0.005f);
    g_sim.room_temp = fmaxf(17.0f, fminf(26.0f, g_sim.room_temp));

    // Baby body temperature: converges to target with thermal inertia ~120s
    state_physics_t ph = get_state_physics(g_sim.state);
    float tau_body = 120.0f;
    float alpha_body = dt / (tau_body + dt);
    g_sim.body_temp += (ph.body_temp_target - g_sim.body_temp) * alpha_body;
    g_sim.body_temp += gauss_noise(0.02f);
    g_sim.body_temp = fmaxf(36.0f, fminf(38.0f, g_sim.body_temp));

    // Humidity: base 45%, increases with activity, ventilates slowly
    float hum_target = 45.0f + ph.humidity_delta;
    float tau_hum = 60.0f;
    float alpha_hum = dt / (tau_hum + dt);
    g_sim.humidity += (hum_target - g_sim.humidity) * alpha_hum;
    g_sim.humidity += gauss_noise(0.05f);
    g_sim.humidity = fmaxf(30.0f, fminf(70.0f, g_sim.humidity));

    // Noise: RC-filter with fast attack (~1s) and slow decay (~5s)
    float tau_noise = (ph.noise_target > g_sim.smooth_noise) ? 1.0f : 5.0f;
    float alpha_noise = dt / (tau_noise + dt);
    g_sim.smooth_noise = lp_filter(g_sim.smooth_noise, ph.noise_target, alpha_noise);
    g_sim.smooth_noise += gauss_noise(0.005f);
    g_sim.smooth_noise = fmaxf(0.01f, fminf(0.95f, g_sim.smooth_noise));

    // Motion probability: fast attack (~0.5s), slow decay (~8s)
    float tau_mot = (ph.motion_prob > g_sim.smooth_motion_prob) ? 0.5f : 8.0f;
    float alpha_mot = dt / (tau_mot + dt);
    g_sim.smooth_motion_prob = lp_filter(g_sim.smooth_motion_prob, ph.motion_prob, alpha_mot);
    g_sim.smooth_motion_prob = fmaxf(0.0f, fminf(1.0f, g_sim.smooth_motion_prob));

    // Nightlight: turns on at CRYING_LOUD/SCREAMING, turns off ~30s after calming
    if (g_sim.state == BABY_CRYING_LOUD || g_sim.state == BABY_SCREAMING) {
        g_sim.nightlight_on = true;
        g_sim.nightlight_timer = 0;
    } else if (g_sim.nightlight_on) {
        g_sim.nightlight_timer += delta_ms;
        if (g_sim.nightlight_timer > 30000) {  // 30 seconds
            g_sim.nightlight_on = false;
            g_sim.light_lux = 3;  // dark
        }
    }
}

// ---------------------------------------------------------------------------
// Check environmental triggers for baby state
// Too hot/cold -> transition to restless state
// ---------------------------------------------------------------------------
static state_trigger_t check_env_triggers(void) {
    if (g_sim.room_temp > 24.5f && g_sim.state == BABY_SLEEPING_PEACEFUL)
        return TRIGGER_ROOM_TOO_HOT;
    if (g_sim.room_temp < 18.5f && g_sim.state == BABY_SLEEPING_PEACEFUL)
        return TRIGGER_ROOM_TOO_COLD;
    return TRIGGER_TIMEOUT;  // no environmental trigger
}

static void update_baby_state(uint32_t delta_ms) {
    // 1. Update physics (temperature, noise, humidity)
    update_room_physics(delta_ms);

    g_sim.state_timer += delta_ms;

    // 2. Check environmental triggers (don't wait for timer)
    state_trigger_t env_trigger = check_env_triggers();
    if (env_trigger != TRIGGER_TIMEOUT) {
        g_sim.state = BABY_RESTLESS;
        g_sim.state_duration = 5000 + (rand() % 10000);  // 5-15s
        g_sim.last_trigger = env_trigger;
        g_sim.event_count++;
        return;
    }

    // 3. Timer transitions
    if (g_sim.state_timer < g_sim.state_duration) return;

    g_sim.state_timer = 0;

    // State machine with realistic transitions
    switch (g_sim.state) {
        case BABY_SLEEPING_PEACEFUL:
            // Random awakening
            g_sim.state = BABY_STIRRING;
            g_sim.state_duration = 8000 + (rand() % 7000);  // 8-15s
            break;

        case BABY_RESTLESS:
            // Either calm down or escalate
            if (rand() % 100 < 60) {  // 60% chance to calm
                g_sim.state = BABY_SLEEPING_PEACEFUL;
                g_sim.state_duration = 20000 + (rand() % 20000);  // 20-40s
            } else {
                g_sim.state = BABY_STIRRING;
                g_sim.state_duration = 5000 + (rand() % 8000);  // 5-13s
            }
            break;

        case BABY_STIRRING:
            // Either go back to sleep or escalate to fussing
            if (rand() % 100 < 40) {  // 40% chance to sleep
                g_sim.state = BABY_SLEEPING_PEACEFUL;
                g_sim.state_duration = 15000 + (rand() % 15000);  // 15-30s
            } else {
                g_sim.state = BABY_FUSSY;
                g_sim.state_duration = 8000 + (rand() % 7000);  // 8-15s
            }
            break;

        case BABY_FUSSY:
            // Escalate to crying
            g_sim.state = BABY_CRYING_SOFT;
            g_sim.state_duration = 10000 + (rand() % 10000);  // 10-20s
            break;

        case BABY_CRYING_SOFT:
            // Either escalate or de-escalate
            if (rand() % 100 < 70) {  // 70% chance to escalate
                g_sim.state = BABY_CRYING_LOUD;
                g_sim.state_duration = 8000 + (rand() % 12000);  // 8-20s
            } else {
                g_sim.state = BABY_FUSSY;
                g_sim.state_duration = 6000 + (rand() % 8000);  // 6-14s
            }
            break;

        case BABY_CRYING_LOUD:
            // Either escalate or de-escalate
            if (rand() % 100 < 30) {  // 30% chance to escalate
                g_sim.state = BABY_SCREAMING;
                g_sim.state_duration = 5000 + (rand() % 8000);  // 5-13s
            } else {
                g_sim.state = BABY_CRYING_SOFT;
                g_sim.state_duration = 10000 + (rand() % 10000);  // 10-20s
            }
            break;

        case BABY_SCREAMING:
            // Must de-escalate (can't scream forever)
            g_sim.state = BABY_CRYING_LOUD;
            g_sim.state_duration = 12000 + (rand() % 8000);  // 12-20s
            break;

        case BABY_FALLING_ASLEEP:
            // Transition to peaceful sleep
            g_sim.state = BABY_SLEEPING_PEACEFUL;
            g_sim.state_duration = 30000 + (rand() % 30000);  // 30-60s
            break;
    }

    g_sim.last_trigger = TRIGGER_TIMEOUT;
    g_sim.event_count++;
}

// ---------------------------------------------------------------------------
// Sensor data generator - only reads from physical model.
// All physics updates in update_baby_state() -> update_room_physics().
// ---------------------------------------------------------------------------
static void generate_realistic_sensor_data(SensorData* data) {
    // Update physics + state machine (100ms - sensor polling interval)
    update_baby_state(100);

    // Temperature sensor = room temperature + measurement noise
    // Baby influences it indirectly through body; sensor on wall
    data->temperature_c = g_sim.room_temp + gauss_noise(0.05f);

    // Humidity
    data->humidity_percent = g_sim.humidity + gauss_noise(0.3f);

    // Illumination
    data->light_level_lux = g_sim.light_lux;

    // Motion: probability determined by smoothed value from physics
    // Important: motion_detected triggers EARLIER than audio events (see RC-filters)
    data->pir_motion_detected = ((float)(rand() % 1000) / 1000.0f) < g_sim.smooth_motion_prob;
}

// ---------------------------------------------------------------------------
// Audio data generator - reads from same physical model.
// KEY CORRELATION: audio detector triggers AFTER video detector
// ---------------------------------------------------------------------------
static void generate_realistic_audio_data(AudioData* data) {
    state_physics_t ph = get_state_physics(g_sim.state);
    data->noise_level = g_sim.smooth_noise + gauss_noise(0.01f);
    if (data->noise_level < 0.01f) data->noise_level = 0.01f;
    if (data->noise_level > 0.99f) data->noise_level = 0.99f;
    data->voice_activity = (g_sim.state >= BABY_STIRRING) && ((float)(rand() % 100) < 85.0f);
    data->baby_crying = ph.cry && ((float)(rand() % 100) > 10);
    data->screaming = ph.scream && ((float)(rand() % 100) > 5);
    static const int freq_min[] = {100, 200, 400, 600, 800, 1200, 1500, 100};
    static const int freq_range[] = {200, 300, 500, 600, 800, 1000, 2000, 250};
    int si = (int)g_sim.state;
    data->peak_frequency_hz = (uint16_t)(freq_min[si] + (rand() % freq_range[si]));
}

// JSON generation functions
char* generate_sensor_json(const SensorData* data) {
    char* json = malloc(512);
    if (!json) return NULL;
    
    snprintf(json, 512,
        "{"
        "\"temperature\":%.2f,"
        "\"humidity\":%.1f,"
        "\"light\":%d,"
        "\"motion\":%s,"
        "\"timestamp\":%u"
        "}",
        data->temperature_c,
        data->humidity_percent,
        data->light_level_lux,
        data->pir_motion_detected ? "true" : "false",
        get_timestamp_ms()
    );
    
    return json;
}

char* generate_audio_json(const AudioData* data) {
    char* json = malloc(512);
    if (!json) return NULL;
    
    snprintf(json, 512,
        "{"
        "\"noise\":%.3f,"
        "\"voice\":%s,"
        "\"crying\":%s,"
        "\"screaming\":%s,"
        "\"peak_freq\":%u,"
        "\"timestamp\":%u"
        "}",
        data->noise_level,
        data->voice_activity ? "true" : "false",
        data->baby_crying ? "true" : "false",
        data->screaming ? "true" : "false",
        data->peak_frequency_hz,
        get_timestamp_ms()
    );
    
    return json;
}

// Public API implementation
DataAgent* data_agent_create(const AgentConfig* config) {
    DataAgent* agent = malloc(sizeof(DataAgent));
    if (!agent) return NULL;
    
    memset(agent, 0, sizeof(DataAgent));
    agent->config = *config;
    agent->running = false;
    
    // Initialize simulation with some randomness
    g_sim.room_temp += gauss_noise(0.5f);
    g_sim.body_temp += gauss_noise(0.2f);
    g_sim.humidity += gauss_noise(2.0f);
    
    return agent;
}

bool data_agent_start(DataAgent* agent) {
    if (!agent || agent->running) return false;
    
    agent->running = true;
    return true;
}

void data_agent_stop(DataAgent* agent) {
    if (!agent) return;
    agent->running = false;
}

void data_agent_destroy(DataAgent* agent) {
    if (!agent) return;
    free(agent);
}

bool data_agent_is_running(const DataAgent* agent) {
    return agent ? agent->running : false;
}

SensorData data_agent_get_sensor_data(const DataAgent* agent) {
    (void)agent;  // Suppress unused warning
    SensorData data = {0};
    generate_realistic_sensor_data(&data);
    return data;
}

AudioData data_agent_get_audio_data(const DataAgent* agent) {
    (void)agent;  // Suppress unused warning
    AudioData data = {0};
    generate_realistic_audio_data(&data);
    return data;

char* data_agent_get_sensor_json(const DataAgent* agent) {
    SensorData data = data_agent_get_sensor_data(agent);
    return generate_sensor_json(&data);
}

char* data_agent_get_audio_json(const DataAgent* agent) {
    AudioData data = data_agent_get_audio_data(agent);
    return generate_audio_json(&data);
}
