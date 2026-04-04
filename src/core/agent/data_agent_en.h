#ifndef DATA_AGENT_EN_H
#define DATA_AGENT_EN_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// PHYSICAL MODEL OF BABY NURSERY
//
// Key correlation principles:
// 1. Baby body temperature rises during activity/crying
//    and slowly returns to normal (thermal inertia ~120s).
// 2. Room temperature can drift independently and
//    cause baby discomfort (discomfort).
// 3. Humidity correlates with activity (crying + breathing increase it).
// 4. Noise level directly depends on state.
// 5. Motion precedes sound - baby starts moving first,
//    then makes sounds. Audio detector triggers later.
// 6. Room light - night mode (~5 lux); can turn on
//    nightlight during strong crying (parent reaction simulation).
// ============================================================

// Baby sleep state simulation
typedef enum {
    BABY_SLEEPING_PEACEFUL,  // Peaceful sleep
    BABY_RESTLESS,           // Restless sleep (first sign - movement)
    BABY_STIRRING,           // Stirring + first sounds
    BABY_FUSSY,              // Fussy, active vocalization
    BABY_CRYING_SOFT,        // Soft crying
    BABY_CRYING_LOUD,        // Loud crying
    BABY_SCREAMING,          // Screaming
    BABY_FALLING_ASLEEP      // Calming down, falling asleep
} baby_state_t;

// State change reason - used for logging
typedef enum {
    TRIGGER_TIMEOUT,         // State timer expired
    TRIGGER_ROOM_TOO_HOT,    // Room too hot (>24°C)
    TRIGGER_ROOM_TOO_COLD,   // Room too cold (<18°C)
    TRIGGER_RANDOM           // Random transition (hunger, dreams, etc.)
} state_trigger_t;

typedef struct {
    baby_state_t state;
    uint32_t     state_duration;   // ms until next state evaluation
    uint32_t     state_timer;      // accumulated time in current state

    // Physical parameters with their own inertia
    float room_temp;          // air temperature, °C
    float body_temp;          // baby body temperature, °C (normal ~36.6)
    float humidity;           // air humidity, %
    uint16_t light_lux;       // illumination, lux

    // Internal "smoothed" levels (used as target for LP-filter)
    float target_noise;
    float smooth_noise;       // smoothed noise_level (RC-filter)
    float smooth_motion_prob; // motion probability [0..1] (smoothed)

    uint32_t event_count;
    state_trigger_t last_trigger;

    // Flag: parent turned on nightlight (resets after ~30s)
    bool nightlight_on;
    uint32_t nightlight_timer;
} baby_simulation_t;

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

#endif // DATA_AGENT_EN_H
