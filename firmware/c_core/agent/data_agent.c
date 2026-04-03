#include "data_agent.h"
#include "video/v4l2_capture.h"
#include "audio/audio_capture.h"
#include "audio/noise_detection.h"
#include "../ffi/rust_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

struct data_agent {
    agent_config_t config;
    agent_stats_t stats;
    pthread_t worker_thread;
    volatile bool running;
    
    // Component instances
    v4l2_capture_t* camera;
    audio_capture_t* audio;
    noise_detector_t* noise_detector;
    rust_motion_detector_t* motion_detector;
    
    // Callbacks
    sensor_data_callback_t sensor_callback;
    audio_data_callback_t audio_callback;
    motion_alert_callback_t motion_callback;
    system_status_callback_t status_callback;
    void* sensor_user_data;
    void* audio_user_data;
    void* motion_user_data;
    void* status_user_data;
    
    // Internal state
    uint32_t sequence;
    uint8_t* prev_frame;
    size_t prev_frame_size;
};

// Helper function to get current timestamp
static uint32_t get_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// ============================================================
// ФИЗИЧЕСКАЯ МОДЕЛЬ ДЕТСКОЙ КОМНАТЫ
//
// Ключевые принципы корреляции:
//  1. Температура тела ребёнка (body_temp) растёт при активности/плаче
//     и медленно возвращается к норме (тепловая инерция ~тау 120 с).
//  2. Температура воздуха (room_temp) может независимо дрейфовать и
//     сама по себе вызывать беспокойство ребёнка (дискомфорт).
//  3. Влажность коррелирует с активностью (плач + дыхание её повышают).
//  4. Уровень шума (noise_level) напрямую зависит от состояния.
//  5. Движение (motion) опережает звук — ребёнок сначала ворочается,
//     потом начинает издавать звуки. Аудио-детектор срабатывает позже.
//  6. Свет в комнате — ночной режим (≈5 lux); может включиться
//     ночник при сильном плаче (имитация реакции родителя).
// ============================================================

// Baby sleep state simulation
typedef enum {
    BABY_SLEEPING_PEACEFUL,  // Спокойный сон
    BABY_RESTLESS,           // Беспокойный сон (первый признак — движение)
    BABY_STIRRING,           // Ворочается + первые звуки
    BABY_FUSSY,              // Капризничает, активная вокализация
    BABY_CRYING_SOFT,        // Тихий плач
    BABY_CRYING_LOUD,        // Громкий плач
    BABY_SCREAMING,          // Крик
    BABY_FALLING_ASLEEP      // Успокаивается, засыпает
} baby_state_t;

// Причина смены состояния — используется для логирования
typedef enum {
    TRIGGER_TIMEOUT,         // Истёк таймер состояния
    TRIGGER_ROOM_TOO_HOT,    // Жарко в комнате (>24°C)
    TRIGGER_ROOM_TOO_COLD,   // Холодно в комнате (<18°C)
    TRIGGER_RANDOM           // Случайный переход (голод, сны и т.д.)
} state_trigger_t;

typedef struct {
    baby_state_t state;
    uint32_t     state_duration;   // мс до следующей оценки перехода
    uint32_t     state_timer;      // накопленное время в текущем состоянии

    // Физические параметры со своей инерцией
    float room_temp;          // температура воздуха, °C
    float body_temp;          // температура тела ребёнка, °C (норма ~36.6)
    float humidity;           // влажность воздуха, %
    uint16_t light_lux;       // освещённость, lux

    // Внутренние «сглаженные» уровни (используются как target для LP-фильтра)
    float target_noise;
    float smooth_noise;       // сглаженный noise_level (RC-фильтр)
    float smooth_motion_prob; // вероятность движения [0..1] (сглаженная)

    uint32_t event_count;
    state_trigger_t last_trigger;

    // Флаг: родитель включил ночник (сбрасывается через ~30 с)
    bool nightlight_on;
    uint32_t nightlight_timer;
} baby_simulation_t;

static baby_simulation_t g_sim = {
    .state            = BABY_SLEEPING_PEACEFUL,
    .state_duration   = 45000,
    .state_timer      = 0,
    .room_temp        = 22.0f,
    .body_temp        = 36.6f,
    .humidity         = 45.0f,
    .light_lux        = 3,      // почти темно — ночник выключен
    .target_noise     = 0.02f,
    .smooth_noise     = 0.02f,
    .smooth_motion_prob = 0.0f,
    .event_count      = 0,
    .last_trigger     = TRIGGER_TIMEOUT,
    .nightlight_on    = false,
    .nightlight_timer = 0,
};

static void update_baby_state(uint32_t delta_ms);
static void generate_realistic_sensor_data(sensor_data_t* data);
static void generate_realistic_audio_data(audio_data_t* data);

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

// ---------------------------------------------------------------------------
// Вспомогательный LP-фильтр первого порядка (RC-аналог)
//   alpha = dt / (tau + dt),  tau — постоянная времени сглаживания
// ---------------------------------------------------------------------------
static inline float lp_filter(float current, float target, float alpha) {
    return current + alpha * (target - current);
}

// Небольшой гауссовый шум через CLT (сумма равномерных)
static float gauss_noise(float sigma) {
    float s = 0.0f;
    for (int i = 0; i < 6; i++) s += (float)(rand() % 1000) / 1000.0f;
    return (s - 3.0f) * sigma;  // нулевое среднее, СКО ≈ sigma
}

// ---------------------------------------------------------------------------
// Физические параметры по состоянию
//   Возвращает «целевые» значения; реальные медленно к ним стремятся.
// ---------------------------------------------------------------------------
typedef struct {
    float body_temp_target;     // °C
    float humidity_delta;       // прибавка к базовой влажности, %
    float noise_target;         // [0..1]
    float motion_prob;          // вероятность движения [0..1]
    bool  cry;
    bool  scream;
} state_physics_t;

static state_physics_t get_state_physics(baby_state_t s) {
    switch (s) {
        case BABY_SLEEPING_PEACEFUL: return (state_physics_t){36.6f,  0.0f, 0.02f, 0.00f, false, false};
        case BABY_RESTLESS:          return (state_physics_t){36.7f,  0.5f, 0.08f, 0.15f, false, false};
        case BABY_STIRRING:          return (state_physics_t){36.8f,  1.0f, 0.16f, 0.35f, false, false};
        case BABY_FUSSY:             return (state_physics_t){36.9f,  1.5f, 0.28f, 0.55f, false, false};
        case BABY_CRYING_SOFT:       return (state_physics_t){37.0f,  2.0f, 0.48f, 0.65f, true,  false};
        case BABY_CRYING_LOUD:       return (state_physics_t){37.2f,  3.0f, 0.68f, 0.78f, true,  false};
        case BABY_SCREAMING:         return (state_physics_t){37.4f,  4.0f, 0.88f, 0.90f, true,  true };
        case BABY_FALLING_ASLEEP:    return (state_physics_t){36.7f,  0.5f, 0.05f, 0.05f, false, false};
        default:                     return (state_physics_t){36.6f,  0.0f, 0.02f, 0.00f, false, false};
    }
}

// ---------------------------------------------------------------------------
// Обновление физики комнаты и тела (вызывается каждые delta_ms мс)
// ---------------------------------------------------------------------------
static void update_room_physics(uint32_t delta_ms) {
    float dt = (float)delta_ms / 1000.0f;  // секунды

    // Температура воздуха: медленный суточный дрейф + слабый шум
    // Иногда (раз в ~5 мин) комната начинает постепенно холодеть или теплеть
    static float room_temp_drift = 0.0f;
    static uint32_t drift_timer = 0;
    drift_timer += delta_ms;
    if (drift_timer > 300000) {  // каждые 5 минут меняем дрейф
        drift_timer = 0;
        room_temp_drift = gauss_noise(0.003f);  // °C/с
    }
    g_sim.room_temp += room_temp_drift * dt + gauss_noise(0.005f);
    g_sim.room_temp = fmaxf(17.0f, fminf(26.0f, g_sim.room_temp));

    // Температура тела ребёнка: стремится к target с тепловой инерцией ~120 с
    state_physics_t ph = get_state_physics(g_sim.state);
    float tau_body = 120.0f;
    float alpha_body = dt / (tau_body + dt);
    g_sim.body_temp = lp_filter(g_sim.body_temp, ph.body_temp_target, alpha_body);
    g_sim.body_temp += gauss_noise(0.02f);
    g_sim.body_temp = fmaxf(36.0f, fminf(38.0f, g_sim.body_temp));

    // Влажность: базовая 45%, растёт при активности, вентилируется медленно
    float hum_target = 45.0f + ph.humidity_delta;
    float tau_hum = 60.0f;
    float alpha_hum = dt / (tau_hum + dt);
    g_sim.humidity = lp_filter(g_sim.humidity, hum_target, alpha_hum);
    g_sim.humidity += gauss_noise(0.05f);
    g_sim.humidity = fmaxf(30.0f, fminf(70.0f, g_sim.humidity));

    // Шум: RC-фильтр с быстрой атакой (~1 с) и медленным спадом (~5 с)
    float tau_noise = (ph.noise_target > g_sim.smooth_noise) ? 1.0f : 5.0f;
    float alpha_noise = dt / (tau_noise + dt);
    g_sim.smooth_noise = lp_filter(g_sim.smooth_noise, ph.noise_target, alpha_noise);
    g_sim.smooth_noise += gauss_noise(0.005f);
    g_sim.smooth_noise = fmaxf(0.01f, fminf(0.95f, g_sim.smooth_noise));

    // Вероятность движения: быстрая атака (~0.5 с), медленный спад (~8 с)
    float tau_mot = (ph.motion_prob > g_sim.smooth_motion_prob) ? 0.5f : 8.0f;
    float alpha_mot = dt / (tau_mot + dt);
    g_sim.smooth_motion_prob = lp_filter(g_sim.smooth_motion_prob, ph.motion_prob, alpha_mot);
    g_sim.smooth_motion_prob = fmaxf(0.0f, fminf(1.0f, g_sim.smooth_motion_prob));

    // Ночник: включается при CRYING_LOUD/SCREAMING, гаснет через ~30 с после успокоения
    if (g_sim.state == BABY_CRYING_LOUD || g_sim.state == BABY_SCREAMING) {
        g_sim.nightlight_on = true;
        g_sim.nightlight_timer = 0;
    } else if (g_sim.nightlight_on) {
        g_sim.nightlight_timer += delta_ms;
        if (g_sim.nightlight_timer > 30000) {
            g_sim.nightlight_on = false;
            g_sim.nightlight_timer = 0;
        }
    }
    g_sim.light_lux = g_sim.nightlight_on ? (uint16_t)(40 + gauss_noise(5.0f)) : (uint16_t)(3 + gauss_noise(1.0f));
    if (g_sim.light_lux < 1) g_sim.light_lux = 1;
}

// ---------------------------------------------------------------------------
// Проверка экологических триггеров для состояния ребёнка
// Слишком жарко/холодно → переходим в беспокойное состояние
// ---------------------------------------------------------------------------
static state_trigger_t check_env_triggers(void) {
    if (g_sim.room_temp > 24.5f && g_sim.state == BABY_SLEEPING_PEACEFUL)
        return TRIGGER_ROOM_TOO_HOT;
    if (g_sim.room_temp < 18.5f && g_sim.state == BABY_SLEEPING_PEACEFUL)
        return TRIGGER_ROOM_TOO_COLD;
    return TRIGGER_TIMEOUT;  // нет экологического триггера
}

static void update_baby_state(uint32_t delta_ms) {
    // 1. Обновляем физику (температура, шум, влажность)
    update_room_physics(delta_ms);

    g_sim.state_timer += delta_ms;

    // 2. Проверяем экологические триггеры (не ждём таймера)
    state_trigger_t env_trigger = check_env_triggers();
    if (env_trigger != TRIGGER_TIMEOUT) {
        g_sim.state = BABY_RESTLESS;
        g_sim.state_duration = 20000;
        g_sim.state_timer = 0;
        g_sim.last_trigger = env_trigger;
        printf("[SIM] Environmental trigger: %s (room_temp=%.1f°C)\n",
               env_trigger == TRIGGER_ROOM_TOO_HOT ? "TOO HOT" : "TOO COLD",
               g_sim.room_temp);
        return;
    }

    // 3. Таймерные переходы
    if (g_sim.state_timer < g_sim.state_duration) return;

    g_sim.state_timer = 0;
    g_sim.event_count++;
    g_sim.last_trigger = TRIGGER_TIMEOUT;

    switch (g_sim.state) {
        case BABY_SLEEPING_PEACEFUL:
            if (g_sim.event_count >= 2) {
                g_sim.state = BABY_RESTLESS;
                g_sim.state_duration = 20000;
                printf("[SIM] Baby getting restless...\n");
            } else {
                g_sim.state_duration = 45000;
            }
            break;

        case BABY_RESTLESS:
            if (rand() % 100 < 60) {
                g_sim.state = BABY_SLEEPING_PEACEFUL;
                g_sim.state_duration = 40000;
                printf("[SIM] Baby calmed down.\n");
            } else {
                g_sim.state = BABY_STIRRING;
                g_sim.state_duration = 15000;
                printf("[SIM] Baby starting to stir...\n");
            }
            break;

        case BABY_STIRRING:
            if (rand() % 100 < 50) {
                g_sim.state = BABY_FALLING_ASLEEP;
                g_sim.state_duration = 25000;
                printf("[SIM] Baby falling back asleep...\n");
            } else {
                g_sim.state = BABY_FUSSY;
                g_sim.state_duration = 12000;
                printf("[SIM] Baby getting fussy...\n");
            }
            break;

        case BABY_FUSSY:
            if (rand() % 100 < 40) {
                g_sim.state = BABY_FALLING_ASLEEP;
                g_sim.state_duration = 30000;
                printf("[SIM] Baby calming down.\n");
            } else {
                g_sim.state = BABY_CRYING_SOFT;
                g_sim.state_duration = 10000;
                printf("[SIM] Baby starting to cry softly...\n");
            }
            break;

        case BABY_CRYING_SOFT:
            if (rand() % 100 < 30) {
                g_sim.state = BABY_FALLING_ASLEEP;
                g_sim.state_duration = 35000;
                printf("[SIM] Baby stopped crying.\n");
            } else {
                g_sim.state = BABY_CRYING_LOUD;
                g_sim.state_duration = 8000;
                printf("[SIM] Baby crying louder!\n");
            }
            break;

        case BABY_CRYING_LOUD:
            if (rand() % 100 < 25) {
                g_sim.state = BABY_FALLING_ASLEEP;
                g_sim.state_duration = 40000;
                printf("[SIM] Baby calming down from loud cry.\n");
            } else {
                g_sim.state = BABY_SCREAMING;
                g_sim.state_duration = 5000;
                printf("[SIM] Baby SCREAMING!\n");
            }
            break;

        case BABY_SCREAMING:
            g_sim.state = BABY_FALLING_ASLEEP;
            g_sim.state_duration = 45000;
            printf("[SIM] Baby exhausted, falling asleep...\n");
            break;

        case BABY_FALLING_ASLEEP:
            g_sim.state = BABY_SLEEPING_PEACEFUL;
            g_sim.state_duration = 50000;
                printf("[SIM] Baby in peaceful sleep.\n");
                break;
        }

        printf("[SIM] State -> %s (event=%d, room=%.1f°C, body=%.1f°C)\n",
               get_state_name(g_sim.state), g_sim.event_count,
               g_sim.room_temp, g_sim.body_temp);
}

// ---------------------------------------------------------------------------
// Генератор данных датчиков — только читает из физической модели.
// Вся физика обновляется в update_baby_state() → update_room_physics().
// ---------------------------------------------------------------------------
static void generate_realistic_sensor_data(sensor_data_t* data) {
    // Обновление физики + машины состояний (100 мс — интервал опроса датчиков)
    update_baby_state(100);

    // Температура датчика = температура воздуха в комнате + шум измерения
    // Ребёнок влияет на неё косвенно через тело; датчик на стене
    data->temperature = g_sim.room_temp + gauss_noise(0.05f);

    // Влажность
    data->humidity = g_sim.humidity + gauss_noise(0.3f);

    // Освещённость
    data->light_level = g_sim.light_lux;

    // Движение: вероятность определяется сглаженным значением из физики
    // Важно: motion_detected срабатывает РАНЬШЕ аудио-событий (см. RC-фильтры)
    data->motion_detected = ((float)(rand() % 1000) / 1000.0f) < g_sim.smooth_motion_prob;
}

// ---------------------------------------------------------------------------
// Генератор аудио-данных — читает из той же физической модели.
// КЛЮЧЕВАЯ КОРРЕЛЯЦИЯ: аудио-детектор срабатывает ПОЗЖЕ видео-детектора,
// потому что smooth_noise имеет бо́льшую постоянную нарастания (~1 с)
// по сравнению со smooth_motion_prob (~0.5 с).
// ---------------------------------------------------------------------------
static void generate_realistic_audio_data(audio_data_t* data) {
    state_physics_t ph = get_state_physics(g_sim.state);

    // noise_level берём из сглаженной модели + небольшой измерительный шум
    data->noise_level = g_sim.smooth_noise + gauss_noise(0.01f);
    if (data->noise_level < 0.01f) data->noise_level = 0.01f;
    if (data->noise_level > 0.99f) data->noise_level = 0.99f;

    // Голосовая активность — есть начиная с STIRRING
    // Вероятность пропорциональна уровню шума выше порога
    float voice_prob = (g_sim.smooth_noise - 0.10f) * 2.0f;
    if (voice_prob < 0.0f) voice_prob = 0.0f;
    if (voice_prob > 1.0f) voice_prob = 1.0f;
    data->voice_activity = ((float)(rand() % 1000) / 1000.0f) < voice_prob;

    // Детектор плача срабатывает при реальном плаче, но с задержкой и
    // иногда пропускает событие (false negative ~10%)
    data->baby_crying = ph.cry && ((float)(rand() % 100) > 10);

    // Детектор крика
    data->screaming = ph.scream && ((float)(rand() % 100) > 5);

    // Пиковая частота определяется состоянием:
    //   дыхание ~100-300 Гц → хныканье ~400-900 Гц → плач ~800-2000 Гц → крик ~1500-3500 Гц
    static const int freq_min[] = {100, 200, 400, 600, 800, 1200, 1500, 100};
    static const int freq_range[] = {200, 300, 500, 600, 800, 1000, 2000, 250};
    int si = (int)g_sim.state;
    data->peak_frequency = (uint16_t)(freq_min[si] + (rand() % freq_range[si]));
}

// Worker thread function
static void* agent_worker_thread(void* arg) {
    data_agent_t* agent = (data_agent_t*)arg;
    
    while (agent->running) {
        uint32_t timestamp = get_timestamp_ms();
        
        // Update sensor data
        if (agent->config.enable_sensors) {
            sensor_data_t sensor_data;
            
            if (agent->config.enable_simulation) {
                generate_realistic_sensor_data(&sensor_data);
            } else {
                // Read from real sensors - not implemented yet
                sensor_data.temperature = 22.0f + (rand() % 100) * 0.1f;
                sensor_data.humidity = 45.0f + (rand() % 200) * 0.1f;
                sensor_data.light_level = 300 + (rand() % 400);
                sensor_data.motion_detected = (rand() % 100) < 5;
            }
            
            if (agent->sensor_callback) {
                agent->sensor_callback(&sensor_data, agent->sensor_user_data);
            }
            agent->stats.sensor_updates++;
        }
        
        // Update audio data
        if (agent->config.enable_audio) {
            audio_data_t audio_data;
            
            if (agent->config.enable_simulation) {
                generate_realistic_audio_data(&audio_data);
            } else if (agent->audio && agent->noise_detector) {
                // Read from real audio
                uint8_t audio_samples[1024];
                int samples_read = audio_read_samples(agent->audio, audio_samples, sizeof(audio_samples));
                if (samples_read > 0) {
                    noise_metrics_t metrics = noise_detector_analyze(agent->noise_detector, audio_samples, samples_read);
                    audio_data.noise_level = metrics.m_noise_level;
                    audio_data.voice_activity = metrics.m_voice_activity;
                    audio_data.baby_crying = metrics.m_baby_crying_detected;
                    audio_data.screaming = metrics.m_screaming_detected;
                    audio_data.peak_frequency = 1000 + (rand() % 3000);
                }
            }
            
            if (agent->audio_callback) {
                agent->audio_callback(&audio_data, agent->audio_user_data);
            }
            agent->stats.audio_events++;
        }
        
        // Process video and motion detection
        if (agent->config.enable_camera && agent->camera) {
            size_t frame_size = 0;
            uint8_t* frame_data = v4l2_read_frame(agent->camera, &frame_size);
            
            if (frame_data && frame_size > 0) {
                agent->stats.frames_processed++;
                
                // Motion detection
                if (agent->motion_detector && agent->prev_frame) {
                    motion_result_t result = rust_detector_detect_motion_advanced(
                        agent->motion_detector,
                        (const uint8_t*)agent->prev_frame, (uint32_t)agent->prev_frame_size,
                        (const uint8_t*)frame_data, (uint32_t)frame_size,
                        agent->config.motion_threshold
                    );
                    
                    if (result.motion_detected) {
                        motion_alert_t alert;
                        alert.motion_level = result.motion_level;
                        alert.x_coord = 320; // Default center
                        alert.y_coord = 240; // Default center
                        alert.frame_number = agent->stats.frames_processed;
                        alert.confidence = (uint8_t)(result.motion_level * 100);
                        alert.reserved = 0;
                        
                        if (agent->motion_callback) {
                            agent->motion_callback(&alert, agent->motion_user_data);
                        }
                        agent->stats.motion_events++;
                    }
                }
                
                // Store previous frame
                free(agent->prev_frame);
                agent->prev_frame = malloc(frame_size);
                if (agent->prev_frame) {
                    memcpy(agent->prev_frame, frame_data, frame_size);
                    agent->prev_frame_size = frame_size;
                }
            }
        }
        
        // Send system status
        if (agent->status_callback) {
            system_status_t status;
            status.cpu_usage = 20 + (rand() % 30);
            status.memory_usage = 30 + (rand() % 40);
            status.fps = 25 + (rand() % 10);
            status.frames_processed = agent->stats.frames_processed;
            status.uptime = get_timestamp_ms() - agent->stats.start_time;
            status.camera_active = agent->config.enable_camera;
            status.recording_active = false; // TODO: implement recording
            status.reserved = 0;
            
            agent->status_callback(&status, agent->status_user_data);
        }
        
        // Sleep for update interval
        usleep(agent->config.update_interval_ms * 1000);
    }
    
    return NULL;
}

// Public API implementation
data_agent_t* data_agent_create(const agent_config_t* config) {
    data_agent_t* agent = calloc(1, sizeof(data_agent_t));
    if (!agent) return NULL;
    
    memcpy(&agent->config, config, sizeof(agent_config_t));
    agent->stats.start_time = get_timestamp_ms();
    agent->sequence = 0;
    
    // Initialize components
    if (config->enable_camera && !config->enable_simulation) {
        agent->camera = v4l2_create(config->video_source);
        if (agent->camera) {
            v4l2_initialize(agent->camera, 640, 480);
            v4l2_start_capture(agent->camera);
        }
        
        agent->motion_detector = rust_detector_create();
        if (agent->motion_detector) {
            rust_detector_initialize(agent->motion_detector);
        }
    }
    
    if (config->enable_audio && !config->enable_simulation) {
        agent->audio = audio_create_mock();
        if (agent->audio) {
            audio_initialize(agent->audio, 44100, 1);
            audio_start_capture(agent->audio);
        }
        
        agent->noise_detector = noise_detector_create(44100);
        if (agent->noise_detector) {
            noise_detector_initialize(agent->noise_detector);
        }
    }
    
    if (config->enable_sensors && !config->enable_simulation) {
        // TODO: Implement real sensor initialization
        printf("Sensors enabled but not implemented in simulation mode\n");
    }
    
    return agent;
}

void data_agent_destroy(data_agent_t* agent) {
    if (!agent) return;
    
    data_agent_stop(agent);
    
    // Cleanup components
    if (agent->camera) v4l2_destroy(agent->camera);
    if (agent->motion_detector) rust_detector_destroy(agent->motion_detector);
    if (agent->audio) audio_destroy(agent->audio);
    if (agent->noise_detector) noise_detector_destroy(agent->noise_detector);
    
    free(agent->prev_frame);
    free(agent);
}

bool data_agent_start(data_agent_t* agent) {
    if (!agent || agent->running) return false;
    
    agent->running = true;
    return pthread_create(&agent->worker_thread, NULL, agent_worker_thread, agent) == 0;
}

void data_agent_stop(data_agent_t* agent) {
    if (!agent || !agent->running) return;
    
    agent->running = false;
    pthread_join(agent->worker_thread, NULL);
}

bool data_agent_is_running(const data_agent_t* agent) {
    return agent ? agent->running : false;
}

// Callback registration
void data_agent_set_sensor_callback(data_agent_t* agent, sensor_data_callback_t callback, void* user_data) {
    if (agent) {
        agent->sensor_callback = callback;
        agent->sensor_user_data = user_data;
    }
}

void data_agent_set_audio_callback(data_agent_t* agent, audio_data_callback_t callback, void* user_data) {
    if (agent) {
        agent->audio_callback = callback;
        agent->audio_user_data = user_data;
    }
}

void data_agent_set_motion_callback(data_agent_t* agent, motion_alert_callback_t callback, void* user_data) {
    if (agent) {
        agent->motion_callback = callback;
        agent->motion_user_data = user_data;
    }
}

void data_agent_set_status_callback(data_agent_t* agent, system_status_callback_t callback, void* user_data) {
    if (agent) {
        agent->status_callback = callback;
        agent->status_user_data = user_data;
    }
}

// Statistics
agent_stats_t data_agent_get_stats(const data_agent_t* agent) {
    return agent ? agent->stats : (agent_stats_t){0};
}

void data_agent_reset_stats(data_agent_t* agent) {
    if (agent) {
        memset(&agent->stats, 0, sizeof(agent_stats_t));
        agent->stats.start_time = get_timestamp_ms();
    }
}

// Configuration
void data_agent_set_motion_threshold(data_agent_t* agent, float threshold) {
    if (agent) {
        agent->config.motion_threshold = threshold;
    }
}

void data_agent_set_audio_threshold(data_agent_t* agent, float threshold) {
    if (agent) {
        agent->config.audio_threshold = threshold;
    }
}

void data_agent_set_update_interval(data_agent_t* agent, uint32_t interval_ms) {
    if (agent) {
        agent->config.update_interval_ms = interval_ms;
    }
}
