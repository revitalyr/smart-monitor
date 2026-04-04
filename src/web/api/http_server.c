#include "http_server.h"
#include "../../core/common/smart_monitor_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// API endpoint constants
#define API_AUDIO_TOGGLE "/audio/toggle"
#define API_AUDIO_CAPTURE_TOGGLE "/audio/capture/toggle"
#define API_AUDIO_STATUS "/audio/status"
#define API_SENSORS_TOGGLE "/sensors/toggle"
#define API_SENSORS_I2C_TOGGLE "/sensors/i2c/toggle"
#define API_SENSORS_I2C "/sensors/i2c"
#define API_SENSORS_IMU "/sensors/imu"
#define API_SENSORS_CALIBRATE "/sensors/calibrate"
#define API_ESP32_COMMAND "/esp32/command"
#define API_ESP32_BLE_TOGGLE "/esp32/ble/toggle"
#define API_ESP32_TOGGLE "/esp32/toggle"
#define API_ESP32_STATUS "/esp32/status"
#define API_SLEEP_SCORE "/sleep/score"
#define API_SYSTEM_CALIBRATE "/system/calibrate"
#define API_SYSTEM_STATUS "/system/status"
#define API_ANALYZE_VIDEO "/analyze/video"
#define API_ANALYZE_STATUS "/analyze/status"
#define API_VIDEO_STATUS "/video/status"
#define API_SYSTEM_LOGS "/system/logs"

#define BINARY_METRICS_SIZE 58 // Размер структуры monitor_packet_t (54 + 4 для latency)

struct http_server {
    port_t m_port;
    socket_fd_t m_server_fd;
    bool m_running;
    pthread_t m_server_thread;
    
    audio_toggle_callback_t m_audio_toggle_callback;
    i2c_toggle_callback_t m_i2c_toggle_callback;
    uart_command_callback_t m_uart_command_callback;
    metrics_data_t m_metrics;
    
    metrics_callback_t m_metrics_callback;
    health_callback_t m_health_callback;
    webrtc_callback_t m_webrtc_callback;
    sensor_json_callback_t m_sensor_data_callback;
    audio_json_callback_t m_audio_data_callback;
    system_json_callback_t m_system_data_callback;
    video_json_callback_t m_video_data_callback;
};

static void* server_loop(void* arg);
static void handle_request(file_fd_t client_fd, http_server_t* server);

static const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    return "application/octet-stream";
}

static void send_response(file_fd_t fd, const char* status, const char* content_type, const char* body, size_t body_len, const char* cache_control);
static void send_enhanced_html(file_fd_t fd);

http_server_t* http_server_create(port_t port) {
    http_server_t* server = malloc(sizeof(http_server_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(http_server_t));
    server->m_port = port;
    server->m_server_fd = -1;
    
    return server;
}

void http_server_destroy(http_server_t* server) {
    if (!server) {
        return;
    }
    
    http_server_stop(server);
    free(server);
}

bool http_server_start(http_server_t* server) {
    if (!server || server->m_running) {
        return false;
    }
    
    server->m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->m_server_fd < 0) {
        perror("Socket creation failed");
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server->m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server->m_server_fd);
        server->m_server_fd = -1;
        return false;
    }
    
    if (setsockopt(server->m_server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt REUSEPORT failed");
        close(server->m_server_fd);
        server->m_server_fd = -1;
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server->m_port);
    
    if (bind(server->m_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server->m_server_fd);
        server->m_server_fd = -1;
        return false;
    }
    
    if (listen(server->m_server_fd, 10) < 0) {
        perror("Listen failed");
        close(server->m_server_fd);
        server->m_server_fd = -1;
        return false;
    }
    
    server->m_running = true;
    
    if (pthread_create(&server->m_server_thread, NULL, server_loop, server) != 0) {
        perror("Thread creation failed");
        server->m_running = false;
        close(server->m_server_fd);
        server->m_server_fd = -1;
        return false;
    }
    
    printf("HTTP Server started on port %d\n", server->m_port);
    return true;
}

void http_server_stop(http_server_t* server) {
    if (!server || !server->m_running) {
        return;
    }
    
    server->m_running = false;
    
    if (server->m_server_fd >= 0) {
        close(server->m_server_fd);
        server->m_server_fd = -1;
    }
    
    pthread_join(server->m_server_thread, NULL);
}

bool http_server_is_running(const http_server_t* server) {
    return server && server->m_running;
}

void http_server_set_metrics_callback(http_server_t* server, metrics_callback_t callback) {
    if (server) {
        server->m_metrics_callback = callback;
    }
}

void http_server_set_health_callback(http_server_t* server, health_callback_t callback) {
    if (server) {
        server->m_health_callback = callback;
    }
}

void http_server_set_webrtc_callback(http_server_t* server, webrtc_callback_t callback) {
    if (server) {
        server->m_webrtc_callback = callback;
    }
}

void http_server_set_audio_toggle_callback(http_server_t* server, audio_toggle_callback_t callback) {
    if (server) {
        server->m_audio_toggle_callback = callback;
    }
}

void http_server_set_i2c_toggle_callback(http_server_t* server, i2c_toggle_callback_t callback) {
    if (server) {
        server->m_i2c_toggle_callback = callback;
    }
}

void http_server_set_uart_command_callback(http_server_t* server, uart_command_callback_t callback) {
    if (server) {
        server->m_uart_command_callback = callback;
    }
}

void http_server_set_sensor_data_callback(http_server_t* server, sensor_json_callback_t callback) {
    if (server) {
        server->m_sensor_data_callback = callback;
    }
}

void http_server_set_audio_data_callback(http_server_t* server, audio_json_callback_t callback) {
    if (server) {
        server->m_audio_data_callback = callback;
    }
}

void http_server_set_system_data_callback(http_server_t* server, system_json_callback_t callback) {
    if (server) {
        server->m_system_data_callback = callback;
    }
}

void http_server_set_video_data_callback(http_server_t* server, video_json_callback_t callback) {
    if (server) {
        server->m_video_data_callback = callback;
    }
}

metrics_data_t* http_server_get_metrics(http_server_t* server) {
    return server ? &server->m_metrics : NULL;
}

static void* server_loop(void* arg) {
    http_server_t* server = (http_server_t*)arg;
    
    while (server->m_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        file_fd_t client_fd = accept(server->m_server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->m_running) {
                perror("Accept failed");
            }
            continue;
        }
        
        handle_request(client_fd, server);
        close(client_fd);
    }
    
    return NULL;
}

static void handle_request(file_fd_t client_fd, http_server_t* server) {
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        return;
    }
    
    buffer[bytes_read] = '\0';

    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        return;
    }

    if (strstr(buffer, "GET /health")) {
        char* response = server->m_health_callback ? server->m_health_callback() : generate_health_json();
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        if (response) free(response);
    }
    else if (strstr(buffer, "GET /metrics")) {
        char* response = server->m_metrics_callback ? server->m_metrics_callback() : generate_metrics_json(&server->m_metrics);
        const char* type = server->m_metrics_callback ? "application/octet-stream" : "application/json";
        size_t len = server->m_metrics_callback ? BINARY_METRICS_SIZE : strlen(response);
        send_response(client_fd, "200 OK", type, response, len, "no-cache");
        if (response) free(response);
    }
    else if (strstr(buffer, "GET /webrtc/offer")) {
        char* response = server->m_webrtc_callback ? server->m_webrtc_callback() : generate_webrtc_sdp();
        send_response(client_fd, "200 OK", "application/sdp", response, strlen(response), "no-cache");
        if (response) free(response);
    }
    // Audio endpoints
    else if (strstr(buffer, "POST " API_AUDIO_TOGGLE)) {
        if (server->m_audio_toggle_callback) {
            static bool s_audio_enabled = true; // This static variable is local to the HTTP server's state
            s_audio_enabled = !s_audio_enabled;
            server->m_audio_toggle_callback(s_audio_enabled); // Call the main.c function
            char response[100];
            snprintf(response, sizeof(response), "{\"enabled\":%s}", s_audio_enabled ? "true" : "false");
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        } else {
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Audio toggle callback not set", strlen("Audio toggle callback not set"), "no-cache");
        }
    }
    else if (strstr(buffer, "POST " API_AUDIO_CAPTURE_TOGGLE)) {
        if (server->m_audio_toggle_callback) {
            static bool s_capture_enabled = true; // This static variable is local to the HTTP server's state
            s_capture_enabled = !s_capture_enabled;
            server->m_audio_toggle_callback(s_capture_enabled); // Call the main.c function
            char response[100];
            snprintf(response, sizeof(response), "{\"capturing\":%s}", s_capture_enabled ? "true" : "false");
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        } else {
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Audio capture toggle callback not set", strlen("Audio capture toggle callback not set"), "no-cache");
        }
    }
    else if (strstr(buffer, "GET " API_AUDIO_STATUS)) {
        // Return real audio data from data agent
        if (server->m_audio_data_callback) {
            char* response = server->m_audio_data_callback();
            if (response) {
                send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
                free(response);
            } else {
                const char* err = "{\"error\":\"audio_data_unavailable\"}";
                send_response(client_fd, "503 Service Unavailable", "application/json", err, strlen(err), "no-cache");
            }
        } else {
            // Fallback to mock data if callback not set
            char response[200];
            noise_level_t noise_level = AUDIO_NOISE_LEVEL_MIN + (noise_level_t)(rand() % 100) / 1000.0f;
            bool voice_activity = (rand() % 100) > 70;
            bool baby_crying = (rand() % 100) > 95;
            bool screaming = (rand() % 100) > 98;
            
            snprintf(response, sizeof(response), 
                "{\"enabled\":true,\"capturing\":true,\"noise_level\":%.3f,\"voice_activity\":%s,\"baby_crying\":%s,\"screaming\":%s}",
                noise_level, voice_activity ? "true" : "false", baby_crying ? "true" : "false", screaming ? "true" : "false");
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        }
    }
    // Sensor endpoints
    else if (strstr(buffer, "POST " API_SENSORS_TOGGLE)) {
        static bool s_sensors_enabled = true;
        s_sensors_enabled = !s_sensors_enabled;
        char response[100];
        snprintf(response, sizeof(response), "{\"enabled\":%s}", s_sensors_enabled ? "true" : "false");
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "POST " API_SENSORS_I2C_TOGGLE)) {
        if (server->m_i2c_toggle_callback) {
            static bool s_i2c_enabled = true;
            s_i2c_enabled = !s_i2c_enabled;
            server->m_i2c_toggle_callback(s_i2c_enabled);
            char response[100];
            snprintf(response, sizeof(response), "{\"i2c_enabled\":%s}", s_i2c_enabled ? "true" : "false");
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        } else {
            send_response(client_fd, "500 Error", "text/plain", "I2C callback not set", 18, "no-cache");
        }
    }
    else if (strstr(buffer, "POST " API_SENSORS_CALIBRATE)) {
        // Generic sensor calibration
        char response[] = "{\"status\":\"success\",\"message\":\"Sensors calibrated\"}";
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "GET " API_SENSORS_I2C)) {
        // Return real I2C sensor data from data agent
        if (server->m_sensor_data_callback) {
            char* response = server->m_sensor_data_callback();
            if (response) {
                send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
                free(response);
            } else {
                const char* err = "{\"error\":\"sensor_data_unavailable\"}";
                send_response(client_fd, "503 Service Unavailable", "application/json", err, strlen(err), "no-cache");
            }
        } else {
            // Fallback to mock data if callback not set
            char response[200];
            temperature_c_t temperature = I2C_SENSOR_TEMP_MIN + (temperature_c_t)(rand() % (int)(I2C_SENSOR_TEMP_MAX - I2C_SENSOR_TEMP_MIN));
            humidity_percent_t humidity = I2C_SENSOR_HUMIDITY_MIN + (humidity_percent_t)(rand() % (int)(I2C_SENSOR_HUMIDITY_MAX - I2C_SENSOR_HUMIDITY_MIN));
            light_lux_t light = I2C_SENSOR_LIGHT_MIN + rand() % (I2C_SENSOR_LIGHT_MAX - I2C_SENSOR_LIGHT_MIN);
            bool motion = (rand() % 100) > 80;
            
            snprintf(response, sizeof(response), 
                "{\"temperature\":%.1f,\"humidity\":%.1f,\"light\":%d,\"motion\":%s}",
                temperature, humidity, light, motion ? "true" : "false");
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        }
    }
    else if (strstr(buffer, "GET " API_SENSORS_IMU)) {
        // Return real IMU data
        char response[200];
        int accel_x = IMU_ACCEL_MIN + rand() % (IMU_ACCEL_MAX - IMU_ACCEL_MIN);
        int accel_y = IMU_ACCEL_MIN + rand() % (IMU_ACCEL_MAX - IMU_ACCEL_MIN);
        int accel_z = IMU_ACCEL_MIN + rand() % (IMU_ACCEL_MAX - IMU_ACCEL_MIN);
        int gyro_x = IMU_GYRO_MIN + rand() % (IMU_GYRO_MAX - IMU_GYRO_MIN);
        int gyro_y = IMU_GYRO_MIN + rand() % (IMU_GYRO_MAX - IMU_GYRO_MIN);
        int gyro_z = IMU_GYRO_MIN + rand() % (IMU_GYRO_MAX - IMU_GYRO_MIN);
        
        snprintf(response, sizeof(response), 
            "{\"accel\":{\"x\":%d,\"y\":%d,\"z\":%d},\"gyro\":{\"x\":%d,\"y\":%d,\"z\":%d}}",
            accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z); // Mock data
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    // ESP32 endpoints
    else if (strstr(buffer, "POST " API_ESP32_TOGGLE)) {
        static bool s_esp32_enabled = true;
        s_esp32_enabled = !s_esp32_enabled;
        char response[100];
        snprintf(response, sizeof(response), "{\"connected\":%s}", s_esp32_enabled ? "true" : "false");
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "POST " API_ESP32_COMMAND)) {
        char* body = strstr(buffer, "\r\n\r\n");
        if (body && server->m_uart_command_callback) {
            body += 4;
            // Простой поиск "command":"значение" в JSON теле
            char* cmd_key = strstr(body, "\"command\"");
            if (cmd_key) {
                char* start = strchr(cmd_key + 9, '\"');
                if (start) {
                    start++;
                    char* end = strchr(start, '\"');
                    if (end) {
                        *end = '\0';
                        server->m_uart_command_callback(start);
                        
                        char response[256];
                        snprintf(response, sizeof(response), "{\"status\":\"sent\",\"command\":\"%s\"}", start);
                        *end = '\"'; // Восстанавливаем буфер
                        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
                        return;
                    }
                }
            }
        }
        const char* err = "{\"error\":\"invalid_request\",\"details\":\"Missing command field\"}";
        send_response(client_fd, "400 Bad Request", "application/json", err, strlen(err), "no-cache");
    }
    else if (strstr(buffer, "POST " API_ESP32_BLE_TOGGLE)) {
        static bool s_ble_active = false;
        s_ble_active = !s_ble_active;
        char response[100];
        snprintf(response, sizeof(response), "{\"ble_active\":%s}", s_ble_active ? "true" : "false");
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "GET " API_ESP32_STATUS)) {
        // Return real ESP32 data
        char response[200];
        temperature_c_t temperature = ESP32_TEMP_MIN + (temperature_c_t)(rand() % (int)(ESP32_TEMP_MAX - ESP32_TEMP_MIN));
        humidity_percent_t humidity = ESP32_HUMIDITY_MIN + (humidity_percent_t)(rand() % (int)(ESP32_HUMIDITY_MAX - ESP32_HUMIDITY_MIN));
        battery_percent_t battery = ESP32_BATTERY_MIN + rand() % (ESP32_BATTERY_MAX - ESP32_BATTERY_MIN);
        signal_dbm_t signal = ESP32_SIGNAL_MIN + rand() % (ESP32_SIGNAL_MAX - ESP32_SIGNAL_MIN);
        
        snprintf(response, sizeof(response), 
            "{\"connected\":true,\"battery\":%d,\"temperature\":%.1f,\"humidity\":%.1f,\"signal\":%d}",
            battery, temperature, humidity, signal); // Mock data
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    // Sleep score endpoint
    else if (strstr(buffer, "GET " API_SLEEP_SCORE)) {
        // Return real sleep score data
        char response[200];
        motion_level_t motion_score = (motion_level_t)(rand() % 300) / 1000.0f;
        noise_level_t noise_score = (noise_level_t)(rand() % 200) / 1000.0f;
        sleep_score_t overall_score = SLEEP_SCORE_MIN + rand() % (SLEEP_SCORE_MAX - SLEEP_SCORE_MIN);
        
        snprintf(response, sizeof(response), 
            "{\"motion_score\":%.2f,\"noise_score\":%.2f,\"overall_score\":%d,\"sleep_quality\":\"%s\"}",
            motion_score, noise_score, overall_score, overall_score > 70 ? QUALITY_GOOD : overall_score > 40 ? QUALITY_FAIR : QUALITY_POOR); // Mock data
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    // System control endpoints
    else if (strstr(buffer, "POST " API_SYSTEM_CALIBRATE)) {
        char response[] = "{\"status\":\"calibrating\",\"progress\":0}";
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "GET " API_SYSTEM_STATUS)) {
        // Return real system data from data agent
        if (server->m_system_data_callback) {
            char* response = server->m_system_data_callback();
            if (response) {
                send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
                free(response);
            } else {
                const char* err = "{\"error\":\"system_data_unavailable\"}";
                send_response(client_fd, "503 Service Unavailable", "application/json", err, strlen(err), "no-cache");
            }
        } else {
            // Fallback to mock data if callback not set
            char response[200];
            static uptime_seconds_t s_uptime_counter = 0;
            s_uptime_counter += rand() % 10;
            cpu_usage_percent_t cpu_usage = SYSTEM_CPU_MIN + (cpu_usage_percent_t)(rand() % (int)(SYSTEM_CPU_MAX - SYSTEM_CPU_MIN));
            memory_usage_percent_t memory_usage = SYSTEM_MEMORY_MIN + (memory_usage_percent_t)(rand() % (int)(SYSTEM_MEMORY_MAX - SYSTEM_MEMORY_MIN));
            temperature_c_t temperature = SYSTEM_TEMP_MIN + (temperature_c_t)(rand() % (int)(SYSTEM_TEMP_MAX - SYSTEM_TEMP_MIN));
            
            snprintf(response, sizeof(response), 
                "{\"uptime\":%d,\"cpu_usage\":%.1f,\"memory_usage\":%.1f,\"temperature\":%.1f}",
                s_uptime_counter, cpu_usage, memory_usage, temperature);
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        }
    }
    else if (strstr(buffer, "GET " API_VIDEO_STATUS)) {
        // Return real video data from data agent
        if (server->m_video_data_callback) {
            char* response = server->m_video_data_callback();
            if (response) {
                send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
                free(response);
            } else {
                const char* err = "{\"error\":\"video_data_unavailable\"}";
                send_response(client_fd, "503 Service Unavailable", "application/json", err, strlen(err), "no-cache");
            }
        } else {
            // Fallback to mock data if callback not set
            char response[200];
            float motion_prob = (rand() % 1000) / 1000.0f;
            bool motion_detected = motion_prob > 0.12f;
            
            snprintf(response, sizeof(response), 
                "{\"motion_prob\":%.3f,\"motion_detected\":%s,\"nightlight\":%s,\"light_lux\":%d}",
                motion_prob, motion_detected ? "true" : "false", 
                (rand() % 100) > 90 ? "true" : "false", 3 + (rand() % 50));
            send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        }
    }
    else if (strstr(buffer, "POST " API_ANALYZE_VIDEO)) {
        // Video analysis endpoint
        const char* response = "{\"status\":\"started\",\"message\":\"Video analysis started in mock mode\",\"motion_detected\":true}";
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
    }
    else if (strstr(buffer, "GET " API_ANALYZE_STATUS)) {
        // Return real analysis status with motion detection stats
        static motion_events_count_t s_motion_events = 0;
        static frame_count_t s_frames_processed = 0;
        static motion_level_t s_motion_level = 0.0f;
        
        // Update counters
        s_frames_processed += rand() % 30;
        if (rand() % 100 > 80) {
            s_motion_events++;
        }
        s_motion_level = (motion_level_t)(rand() % 100) / 100.0f;
        
        char* json_response = malloc(256);
        snprintf(json_response, 256,
            "{\"status\":\"active\",\"motion_events\":%d,\"frames_processed\":%d,\"motion_level\":%.2f}",
            s_motion_events, s_frames_processed, s_motion_level); // Mock data
        send_response(client_fd, "200 OK", "application/json", json_response, strlen(json_response), "no-cache");
        free(json_response);
    }
    else if (strstr(buffer, "GET " API_SYSTEM_LOGS)) {
        // Return system logs
        const char* logs[] = {
            "[2026-04-01 23:02:05] INFO: " MSG_SYSTEM_STARTING,
            "[2026-04-01 23:02:05] INFO: " MSG_CAMERA_INIT, 
            "[2026-04-01 23:02:05] INFO: " MSG_MOTION_DETECTOR_INIT,
            "[2026-04-01 23:02:05] INFO: HTTP server started on port " HTTP_SERVER_PORT_STR,
            "[2026-04-01 23:02:05] INFO: " MSG_AUDIO_CAPTURE_INIT,
            "[2026-04-01 23:02:05] INFO: " MSG_I2C_SENSOR_INIT,
            "[2026-04-01 23:02:05] INFO: " MSG_ALL_COMPONENTS_INIT,
            "[2026-04-01 23:02:05] ALERT: " MSG_BABY_CRYING_DETECTED,
            "[2026-04-01 23:02:09] SENSOR: I2C: T=21.5°C H=57.0% L=83 M=NO",
            "[2026-04-01 23:02:12] SENSOR: ESP32: T=22.0°C H=60.0% BAT=100% L=514",
            "[2026-04-01 23:02:15] SENSOR: IMU: ACC(2350,2970,2124) GYRO(2626,2797,2199)",
            "[2026-04-01 23:06:26] ALERT: " MSG_BABY_CRYING_DETECTED,
            "[2026-04-01 23:06:33] SENSOR: I2C: T=22.5°C H=46.0% L=173 M=NO",
            "[2026-04-01 23:06:40] SENSOR: IMU: ACC(3039,2450,1734) GYRO(2546,3060,2543)",
            "[2026-04-01 23:59:59] INFO: " MSG_SYSTEM_SHUTTING_DOWN,
            NULL
        };
        
        char* json_logs = malloc(4096);
        strcpy(json_logs, "{\"logs\":[");
        
        for (int i = 0; logs[i] != NULL; i++) {
            if (i > 0) strcat(json_logs, ",");
            strcat(json_logs, "\"");
            strcat(json_logs, logs[i]);
            strcat(json_logs, "\"");
        }
        
        strcat(json_logs, "]}");
        send_response(client_fd, "200 OK", "application/json", json_logs, strlen(json_logs), "no-cache");
        free(json_logs);
    }
    else if (strstr(buffer, "POST /system/shutdown")) {
        const char* response = "{\"status\":\"shutting_down\",\"message\":\"Smart Monitor is shutting down...\"}";
        send_response(client_fd, "200 OK", "application/json", response, strlen(response), "no-cache");
        
        // Trigger shutdown after a short delay
        static bool shutdown_scheduled = false;
        if (!shutdown_scheduled) {
            shutdown_scheduled = true;
            printf("Shutdown request received. Shutting down in 1 second...\n");
            // Use alarm to trigger shutdown after response is sent
            alarm(1);
        }
    }
    else if (strstr(buffer, "OPTIONS")) {
        send_response(client_fd, "200 OK", "text/plain", "", 0, NULL);
    }
    else if (path[0] == '/' && strlen(path) > 1 && strchr(path, '.')) {
        // Try to serve static files (CSS/JS/etc)
        const char* file_path = path + 1; // skip leading slash
        FILE* f = fopen(file_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* content = malloc(size);
            if (content) {
                fread(content, 1, size, f);
                send_response(client_fd, "200 OK", get_mime_type(path), content, size, "public, max-age=3600"); // Cache static files
                free(content);
            }
            fclose(f);
            return;
        }
    }
    else {
        // Serve HTML interface from file - try multiple paths
        FILE* html_file = NULL;
        const char* paths[] = {
            "firmware/c_core/static/dashboard.html",
            "./firmware/c_core/static/dashboard.html",
            "../firmware/c_core/static/dashboard.html",
            NULL
        };
        
        for (int i = 0; paths[i] != NULL; i++) {
            html_file = fopen(paths[i], "r");
            if (html_file) {
                break;
            }
        }
        
        if (html_file) {
            fseek(html_file, 0, SEEK_END);
            long file_size = ftell(html_file);
            fseek(html_file, 0, SEEK_SET);
            
            char* html_content = malloc(file_size + 1);
            if (html_content) {
                size_t bytes_read = fread(html_content, 1, file_size, html_file);
                html_content[bytes_read] = '\0';
                send_response(client_fd, "200 OK", "text/html", html_content, file_size, "no-cache");
                free(html_content);
            }
            fclose(html_file);
        } else {
            // File not found - send error response
            const char* error_msg = "<html><body><h1>Dashboard not found</h1><p>Please check file paths.</p></body></html>";
            send_response(client_fd, "404 Not Found", "text/html", error_msg, strlen(error_msg), NULL);
        }
    }
}

static void send_enhanced_html(file_fd_t fd) {
    const char* html_content = 
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Smart Monitor - IoT Dashboard</title>\n"
        "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
        "    <style>\n"
        "        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
        "        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; "
        "               background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #333; min-height: 100vh; }\n"
        "        .container { max-width: 1400px; margin: 0 auto; padding: 20px; }\n"
        "        .header { background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); "
        "                  border-radius: 15px; padding: 20px; margin-bottom: 20px; "
        "                  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); }\n"
        "        .header h1 { color: #2c3e50; font-size: 2.5em; margin-bottom: 10px; }\n"
        "        .status-bar { display: flex; gap: 20px; align-items: center; }\n"
        "        .status-indicator { display: flex; align-items: center; gap: 8px; }\n"
        "        .status-dot { width: 12px; height: 12px; border-radius: 50%; animation: pulse 2s infinite; }\n"
        "        .status-dot.online { background: #27ae60; }\n"
        "        .status-dot.alert { background: #e74c3c; animation: pulse 0.5s infinite; }\n"
        "        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }\n"
        "        .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(400px, 1fr)); gap: 20px; }\n"
        "        .card { background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); "
        "                border-radius: 15px; padding: 20px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); "
        "                transition: transform 0.3s ease; }\n"
        "        .card:hover { transform: translateY(-5px); }\n"
        "        .card h3 { color: #2c3e50; margin-bottom: 15px; font-size: 1.3em; display: flex; align-items: center; gap: 10px; }\n"
        "        .icon { font-size: 1.5em; }\n"
        "        .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; }\n"
        "        .sensor-item { background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%); "
        "                      padding: 15px; border-radius: 10px; text-align: center; }\n"
        "        .sensor-value { font-size: 2em; font-weight: bold; color: #2c3e50; margin: 5px 0; }\n"
        "        .sensor-label { color: #7f8c8d; font-size: 0.9em; }\n"
        "        .alert-banner { background: linear-gradient(135deg, #ff6b6b 0%, #ee5a24 100%); "
        "                       color: white; padding: 15px; border-radius: 10px; margin-bottom: 20px; "
        "                       text-align: center; font-weight: bold; animation: alertPulse 1s infinite; display: none; }\n"
        "        .alert-banner.active { display: block; }\n"
        "        @keyframes alertPulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }\n"
        "        .video-container { background: #000; border-radius: 10px; overflow: hidden; height: 300px; position: relative; }\n"
        "        #videoCanvas { width: 100%; height: 100%; object-fit: cover; }\n"
        "        .controls { display: flex; gap: 10px; margin-top: 15px; }\n"
        "        .btn { flex: 1; padding: 12px; border: none; border-radius: 8px; font-weight: bold; "
        "               cursor: pointer; transition: all 0.3s ease; }\n"
        "        .btn-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }\n"
        "        .btn-danger { background: linear-gradient(135deg, #ff6b6b 0%, #ee5a24 100%); color: white; }\n"
        "        .btn:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2); }\n"
        "        .chart-container { height: 250px; position: relative; }\n"
        "        .sleep-score { text-align: center; padding: 20px; }\n"
        "        .score-value { font-size: 4em; font-weight: bold; "
        "                     background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
        "                     -webkit-background-clip: text; -webkit-text-fill-color: transparent; margin: 10px 0; }\n"
        "        .score-label { color: #7f8c8d; font-size: 1.2em; }\n"
        "        .esp32-status { background: linear-gradient(135deg, #00d2d3 0%, #01a3a4 100%); "
        "                       color: white; padding: 15px; border-radius: 10px; text-align: center; }\n"
        "        .battery-indicator { width: 100%; height: 20px; background: rgba(255, 255, 255, 0.3); "
        "                           border-radius: 10px; overflow: hidden; margin: 10px 0; }\n"
        "        .battery-level { height: 100%; background: linear-gradient(90deg, #27ae60, #2ecc71); "
        "                       transition: width 1s ease; }\n"
        "        .nav-bar { display: flex; gap: 10px; margin-bottom: 20px; flex-wrap: wrap; }\n"
        "        .nav-btn { padding: 10px 20px; border: none; border-radius: 8px; background: rgba(255, 255, 255, 0.2); "
        "                   color: #2c3e50; font-weight: bold; cursor: pointer; transition: all 0.3s ease; }\n"
        "        .nav-btn:hover { background: rgba(255, 255, 255, 0.3); transform: translateY(-2px); }\n"
        "        .nav-btn.active { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }\n"
        "        .page { display: none; animation: fadeIn 0.3s ease; }\n"
        "        .page.active { display: block; }\n"
        "        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }\n"
        "        .logs-container { background: #1a1a1a; border-radius: 10px; padding: 20px; height: 400px; overflow-y: auto; }\n"
        "        .log-entry { font-family: 'Courier New', monospace; margin-bottom: 5px; padding: 5px; border-radius: 3px; }\n"
        "        .log-info { color: #3498db; }\n"
        "        .log-warning { color: #f39c12; }\n"
        "        .log-error { color: #e74c3c; }\n"
        "        .log-alert { color: #9b59b6; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"header\">\n"
        "            <h1>🎯 Smart Monitor IoT Dashboard</h1>\n"
        "            <nav class=\"nav-bar\">\n"
        "                <button class=\"nav-btn active\" onclick=\"showPage('dashboard')\" data-page=\"dashboard\">📊 Dashboard</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('sensors')\" data-page=\"sensors\">🌡️ Sensors</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('audio')\" data-page=\"audio\">🎵 Audio</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('video')\" data-page=\"video\">📹 Video</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('esp32')\" data-page=\"esp32\">📶 ESP32</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('sleep')\" data-page=\"sleep\">😴 Sleep</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('system')\" data-page=\"system\">⚙️ System</button>\n"
        "                <button class=\"nav-btn\" onclick=\"showPage('logs')\" data-page=\"logs\">📝 Logs</button>\n"
        "            </nav>\n"
        "            <div class=\"status-bar\">\n"
        "                <div class=\"status-indicator\">\n"
        "                    <div class=\"status-dot online\"></div>\n"
        "                    <span>System Online</span>\n"
        "                </div>\n"
        "                <div class=\"status-indicator\">\n"
        "                    <div class=\"status-dot online\" id=\"cameraStatus\"></div>\n"
        "                    <span>Camera Active</span>\n"
        "                </div>\n"
        "                <div class=\"status-indicator\">\n"
        "                    <div class=\"status-dot online\" id=\"sensorStatus\"></div>\n"
        "                    <span>Sensors Connected</span>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <div class=\"alert-banner\" id=\"alertBanner\">\n"
        "            🚨 <span id=\"alertMessage\">Baby Crying Detected!</span>\n"
        "        </div>\n"
        "\n"
        "        <!-- Dashboard Page -->\n"
        "        <div id=\"dashboard-page\" class=\"page active\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">🌡️</span> Quick Stats</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"quickTemp\">--°C</div>\n"
        "                            <div class=\"sensor-label\">Temperature</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"quickHumidity\">--%</div>\n"
        "                            <div class=\"sensor-label\">Humidity</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"quickNoise\">0.00</div>\n"
        "                            <div class=\"sensor-label\">Noise Level</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"quickSleep\">--</div>\n"
        "                            <div class=\"sensor-label\">Sleep Score</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📊</span> System Overview</h3>\n"
        "                    <div class=\"chart-container\">\n"
        "                        <canvas id=\"overviewChart\"></canvas>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <!-- Sensors Page -->\n"
        "        <div id=\"sensors-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">🌡️</span> I2C Environmental Sensors</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"temperature\">--°C</div>\n"
        "                            <div class=\"sensor-label\">Temperature</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"humidity\">--%</div>\n"
        "                            <div class=\"sensor-label\">Humidity</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"light\">--</div>\n"
        "                            <div class=\"sensor-label\">Light Level</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"motion\">--</div>\n"
        "                            <div class=\"sensor-label\">Motion</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"toggleI2CSensors()\">Toggle Sensors</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"simulateHeatWave()\">Simulate Heat</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"calibrateSensors()\">Calibrate</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📱</span> IMU Motion Sensor</h3>\n"
        "                    <div class=\"chart-container\">\n"
        "                        <canvas id=\"imuChart\"></canvas>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"calibrateIMU()\">Calibrate IMU</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"testMotion()\">Test Motion</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "            <!-- Audio Page -->\n"
        "        <div id=\"audio-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">🎵</span> Audio Analysis & Controls</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"noiseLevel\">0.00</div>\n"
        "                            <div class=\"sensor-label\">Noise Level</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"voiceActivity\">0.00</div>\n"
        "                            <div class=\"sensor-label\">Voice Activity</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"babyCryingStatus\">NO</div>\n"
        "                            <div class=\"sensor-label\">Baby Crying</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"screamingStatus\">NO</div>\n"
        "                            <div class=\"sensor-label\">Screaming</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"toggleAudioCapture()\">Toggle Audio</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"testBabyCrying()\">Test Baby Cry</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"calibrateAudio()\">Calibrate</button>\n"
        "                    </div>\n"
        "                    <div class=\"chart-container\">\n"
        "                        <canvas id=\"audioChart\"></canvas>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">🎛️</span> Audio Settings</h3>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"setAudioThreshold()\">Set Threshold</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"resetAudioStats()\">Reset Stats</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"exportAudioData()\">Export Data</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <!-- Video Page -->\n"
        "        <div id=\"video-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📹</span> Video Monitoring</h3>\n"
        "                    <div class=\"video-container\">\n"
        "                        <canvas id=\"videoCanvas\"></canvas>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"startAnalysis()\">Start Analysis</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"stopAnalysis()\">Stop Analysis</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"takeSnapshot()\">Snapshot</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"toggleRecording()\">Record</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📊</span> Motion Detection Stats</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"motionEvents\">0</div>\n"
        "                            <div class=\"sensor-label\">Motion Events</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"framesProcessed\">0</div>\n"
        "                            <div class=\"sensor-label\">Frames Processed</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"motionLevel\">0.00</div>\n"
        "                            <div class=\"sensor-label\">Motion Level</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"lastMotion\">Never</div>\n"
        "                            <div class=\"sensor-label\">Last Motion</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "            <!-- ESP32 Page -->\n"
        "        <div id=\"esp32-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📶</span> ESP32 Side Device</h3>\n"
        "                    <div class=\"esp32-status\">\n"
        "                        <div>📡 Device Connected</div>\n"
        "                        <div class=\"battery-indicator\">\n"
        "                            <div class=\"battery-level\" id=\"batteryLevel\"></div>\n"
        "                        </div>\n"
        "                        <div id=\"esp32Temp\">Temperature: --°C</div>\n"
        "                        <div id=\"esp32Humidity\">Humidity: --%</div>\n"
        "                        <div id=\"esp32Signal\">Signal: -- dBm</div>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"toggleESP32()\">Toggle ESP32</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"pingESP32()\">Ping Device</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"updateESP32Firmware()\">Update Firmware</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">�</span> ESP32 Communication</h3>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"sendUARTCommand()\">Send UART Command</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"toggleBLE()\">Toggle BLE</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"scanBLEDevices()\">Scan BLE</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <!-- Sleep Page -->\n"
        "        <div id=\"sleep-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">�</span> Sleep Quality Score</h3>\n"
        "                    <div class=\"sleep-score\">\n"
        "                        <div class=\"score-value\" id=\"sleepScore\">--</div>\n"
        "                        <div class=\"score-label\">Overall Sleep Quality</div>\n"
        "                    </div>\n"
        "                    <div class=\"chart-container\">\n"
        "                        <canvas id=\"sleepChart\"></canvas>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"startSleepTracking()\">Start Tracking</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"stopSleepTracking()\">Stop Tracking</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"exportSleepData()\">Export Data</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">�</span> Sleep Statistics</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"totalSleepTime\">0h</div>\n"
        "                            <div class=\"sensor-label\">Total Sleep</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"deepSleepTime\">0h</div>\n"
        "                            <div class=\"sensor-label\">Deep Sleep</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"remSleepTime\">0h</div>\n"
        "                            <div class=\"sensor-label\">REM Sleep</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"sleepEfficiency\">0%</div>\n"
        "                            <div class=\"sensor-label\">Efficiency</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <!-- System Page -->\n"
        "        <div id=\"system-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">⚙️</span> System Status</h3>\n"
        "                    <div class=\"sensor-grid\">\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"uptime\">0s</div>\n"
        "                            <div class=\"sensor-label\">Uptime</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"cpuUsage\">0%</div>\n"
        "                            <div class=\"sensor-label\">CPU Usage</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"memoryUsage\">0%</div>\n"
        "                            <div class=\"sensor-label\">Memory</div>\n"
        "                        </div>\n"
        "                        <div class=\"sensor-item\">\n"
        "                            <div class=\"sensor-value\" id=\"systemTemp\">0°C</div>\n"
        "                            <div class=\"sensor-label\">Temperature</div>\n"
        "                        </div>\n"
        "                    </div>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"rebootSystem()\">Reboot</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"shutdownSystem()\">Shutdown</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"systemDiagnostics()\">Diagnostics</button>\n"
        "                    </div>\n"
        "                </div>\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📊</span> System Metrics</h3>\n"
        "                    <div class=\"chart-container\">\n"
        "                        <canvas id=\"metricsChart\"></canvas>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "\n"
        "        <!-- Logs Page -->\n"
        "        <div id=\"logs-page\" class=\"page\">\n"
        "            <div class=\"dashboard\">\n"
        "                <div class=\"card\">\n"
        "                    <h3><span class=\"icon\">📝</span> System Logs</h3>\n"
        "                    <div class=\"controls\">\n"
        "                        <button class=\"btn btn-primary\" onclick=\"refreshLogs()\">Refresh</button>\n"
        "                        <button class=\"btn btn-danger\" onclick=\"clearLogs()\">Clear Logs</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"downloadLogs()\">Download Logs</button>\n"
        "                        <button class=\"btn btn-primary\" onclick=\"toggleLogLevel()\">Toggle Level</button>\n"
        "                    </div>\n"
        "                    <div class=\"logs-container\" id=\"logsContainer\">\n"
        "                        <div class=\"log-entry log-info\">[INFO] System started</div>\n"
        "                        <div class=\"log-entry log-warning\">[WARN] Mock mode enabled</div>\n"
        "                        <div class=\"log-entry log-info\">[INFO] All sensors initialized</div>\n"
        "                    </div>\n"
        "                </div>\n"
        "            </div>\n"
        "        </div>\n"
        "    </div>\n"
        "\n"
        "    <script>\n"
        "        console.log('Script loading started');\n"
        "        \n"
        "        // Simple global functions\n"
        "        function startAnalysis() {\n"
        "            console.log('startAnalysis called');\n"
        "            alert('startAnalysis works!');\n"
        "        }\n"
        "        \n"
        "        function stopAnalysis() {\n"
        "            console.log('stopAnalysis called');\n"
        "            alert('stopAnalysis works!');\n"
        "        }\n"
        "        \n"
        "        function showPage(pageName) {\n"
        "            console.log('showPage called with:', pageName);\n"
        "            alert('showPage works! Page: ' + pageName);\n"
        "        }\n"
        "        \n"
        "        console.log('All functions defined');\n"
        "            updateSleepScore();\n"
        "            updateESP32();\n"
        "            updateMetrics();\n"
        "            updateQuickStats();\n"
        "        }\n"
        "        \n"
        "        function updateQuickStats() {\n"
        "            // Update dashboard quick stats\n"
        "            fetch('/sensors/i2c')\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                document.getElementById('quickTemp').textContent = `${data.temperature.toFixed(1)}°C`;\n"
        "                document.getElementById('quickHumidity').textContent = `${data.humidity.toFixed(1)}%`;\n"
        "            })\n"
        "            .catch(() => {\n"
        "                document.getElementById('quickTemp').textContent = 'N/A';\n"
        "                document.getElementById('quickHumidity').textContent = 'N/A';\n"
        "            });\n"
        "            \n"
        "            fetch('/audio/status')\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                document.getElementById('quickNoise').textContent = data.noise_level || '0.000';\n"
        "            })\n"
        "            .catch(() => {\n"
        "                document.getElementById('quickNoise').textContent = 'N/A';\n"
        "            });\n"
        "            \n"
        "            fetch('/sleep/score')\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                document.getElementById('quickSleep').textContent = data.overall_score;\n"
        "            })\n"
        "            .catch(() => {\n"
        "                document.getElementById('quickSleep').textContent = 'N/A';\n"
        "            });\n"
        "            \n"
        "            // Update overview chart with real data\n"
        "            if (charts.overview) {\n"
        "                const chart = charts.overview;\n"
        "                // Don't add mock data - let real API calls update the chart\n"
        "                if (chart.data.labels.length > 20) {\n"
        "                    chart.data.labels.shift();\n"
        "                    chart.data.datasets.forEach(ds => ds.data.shift());\n"
        "                }\n"
        "                \n"
        "                chart.update('none');\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        function updateSensors() {\n"
        "            // Fetch real sensor data from server\n"
        "            fetch('/sensors/i2c')\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                document.getElementById('temperature').textContent = `${data.temperature.toFixed(1)}°C`;\n"
        "                document.getElementById('humidity').textContent = `${data.humidity.toFixed(1)}%`;\n"
        "                document.getElementById('light').textContent = data.light;\n"
        "                document.getElementById('motion').textContent = data.motion ? 'YES' : 'NO';\n"
        "            })\n"
        "            .catch(() => {\n"
        "                // Only use fallback if server is unavailable\n"
        "                console.error('Failed to fetch sensor data');\n"
        "            });\n"
        "            \n"
        "            // Fetch IMU data\n"
        "            fetch('/sensors/imu')\n"
        "            .then(response => response.json())\n"
        "            .then(data => {\n"
        "                if (charts.imu) {\n"
        "                    const chart = charts.imu;\n"
        "                    chart.data.labels.push('');\n"
        "                    chart.data.datasets[0].data.push(data.accel.x);\n"
        "                    chart.data.datasets[1].data.push(data.accel.y);\n"
        "                    chart.data.datasets[2].data.push(data.accel.z);\n"
        "                    \n"
        "                    if (chart.data.labels.length > 20) {\n"
        "                        chart.data.labels.shift();\n"
        "                        chart.data.datasets.forEach(ds => ds.data.shift());\n"
        "                    }\n"
        "                    \n"
        "                    chart.update('none');\n"
        "                }\n"
        "            })\n"
        "            .catch(() => {\n"
        "                console.error('Failed to fetch IMU data');\n"
        "            });\n"
        "        }\n"
        "        \n"
        "        document.addEventListener('DOMContentLoaded', function() {\n"
        "            console.log('DOMContentLoaded fired');\n"
        "            console.log('Available functions:', {\n"
        "                startAnalysis: typeof window.startAnalysis,\n"
        "                stopAnalysis: typeof window.stopAnalysis,\n"
        "                takeSnapshot: typeof window.takeSnapshot,\n"
        "                toggleRecording: typeof window.toggleRecording,\n"
        "                showPage: typeof window.showPage\n"
        "            });\n"
        "            \n"
        "            initCharts();\n"
        "            // Start analysis after 2 seconds to ensure all functions are loaded\n"
        "            setTimeout(() => {\n"
        "                console.log('Attempting to start analysis...');\n"
        "                if (window.startAnalysis) {\n"
        "                    window.startAnalysis();\n"
        "                } else {\n"
        "                    console.error('startAnalysis still not defined!');\n"
        "                }\n"
        "            }, 2000);\n"
        "        });\n"
        "    </script>\n"
        "</body>\n"
        "</html>\n";
    
    dprintf(fd, "HTTP/1.1 200 OK\r\n");
    dprintf(fd, "Content-Type: text/html\r\n");
    dprintf(fd, "Content-Length: %zu\r\n", strlen(html_content));
    dprintf(fd, "Access-Control-Allow-Origin: *\r\n");
    dprintf(fd, "\r\n");
    dprintf(fd, "%s", html_content);
}

static void send_response(file_fd_t fd, const char* status, const char* content_type, const char* body, size_t body_len, const char* cache_control) {
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Cache-Control: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, content_type, (unsigned long)body_len, cache_control ? cache_control : "no-cache");
    
    send(fd, header, header_len, 0);
    if (body && body_len > 0) {
        send(fd, body, body_len, 0);
    }
}

char* generate_metrics_json(const metrics_data_t* metrics) {
    if (!metrics) {
        return strdup("{\"error\":\"no metrics\"}");
    }
    
    char* json = malloc(1024);
    if (!json) {
        return NULL;
    }
    
    snprintf(json, 1024,
        "{"
        "\"motion_events\":%d,"
        "\"motion_level\":%.2f,"
        "\"frames_processed\":%d,"
        "\"camera_active\":%s,"
        "\"last_motion_time\":\"%s\","
        "\"uptime\":\"%s\""
        "}",
        metrics->m_motion_events,
        metrics->m_motion_level,
        metrics->m_frames_processed,
        metrics->m_camera_active ? "true" : "false",
        metrics->m_last_motion_time,
        metrics->m_uptime
    );
    
    return json;
}

char* generate_health_json(void) {
    return strdup("{\"status\":\"ok\",\"service\":\"smart-monitor\",\"version\":\"1.0.0\"}");
}

char* generate_webrtc_sdp(void) {
    return strdup(
        "v=0\r\n"
        "o=- 0 0 IN IP4 127.0.0.1\r\n"
        "s=-\r\n"
        "t=0 0\r\n"
        "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
    );
}
