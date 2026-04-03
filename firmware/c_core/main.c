#include "video/v4l2_capture.h"
#include "net/http_server.h"
#include "net/webrtc_server.h"
#include "ffi/rust_bridge.h"

#ifdef ENABLE_AUDIO
#include "audio/audio_capture.h"
#include "audio/noise_detection.h"
#endif

#ifdef ENABLE_SENSORS
#include "sensors/i2c_sensor.h"
#include "sensors/spi_device.h"
#include "comm/uart_interface.h"
#include "esp32/esp32_device.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define DEFAULT_PORT 8080
#define DEFAULT_DEVICE "/dev/video0"
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

// Бинарный протокол для общения с Web UI
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;           // 0x534D4F4E ('SMON')
    uint32_t uptime_secs;
    
    // Видео и движение
    uint32_t motion_events;
    float    motion_level;
    uint32_t frames_processed;
    float    frame_processing_latency_ms; // New field for latency
    uint8_t  camera_active;

    // Сенсоры I2C
    float    temp;
    float    humidity;
    uint16_t light;
    uint8_t  motion_detected;

    // IMU данные (SPI)
    int16_t  acc_x, acc_y, acc_z;
    
    // Аудио данные
    float    audio_level;
    uint8_t  audio_alert;

    // Данные от ESP32 (UART/BLE)
    float    esp32_temp;
    float    esp32_hum;
    uint8_t  battery;

    // Системные метрики
    uint8_t  cpu_usage;
    uint8_t  mem_usage;
} monitor_packet_t;
#pragma pack(pop)

typedef struct {
    char* device;
    port_t m_port;
    bool m_mock_mode;
    bool m_debug_mode;
    char* m_log_file;
    bool m_syslog_mode;
    char* m_config_file;
} config_t;

static volatile bool running = true;
static monitor_packet_t g_current_packet = { .magic = 0x534D4F4E };
static time_t g_start_time;

#ifdef ENABLE_AUDIO
static bool g_audio_capture_enabled = true; // Global state for audio capture
#endif

static config_t config = {
    .device = DEFAULT_DEVICE,
    .m_port = DEFAULT_PORT,
    .m_mock_mode = false,
    .m_debug_mode = false,
    .m_log_file = NULL,
    .m_syslog_mode = false,
    .m_config_file = "/etc/smartmonitor.conf"
};

bool is_port_in_use(port_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return (result == -1) && (errno == EADDRINUSE);
}

void shutdown_existing_instance(port_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // Send shutdown request
        const char* shutdown_msg = "POST /system/shutdown HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n";
        send(sock, shutdown_msg, strlen(shutdown_msg), 0);
        printf("Sent shutdown request to existing instance on port %d\n", port);
        sleep(2); // Give it time to shutdown
    }
    close(sock);
}

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down Smart Monitor...\n");
    running = false;
    
    // Reinstall signal handler for subsequent Ctrl+C
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void alarm_handler(int sig) {
    (void)sig;
    printf("Alarm triggered. Forcing shutdown...\n");
    running = false;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Smart Monitor - Embedded Video Analytics System\n\n");
    printf("Options:\n");
    printf("  -d, --device DEVICE    Camera device (default: %s)\n", DEFAULT_DEVICE);
    printf("  -p, --port PORT        HTTP port (default: %d)\n", DEFAULT_PORT);
    printf("  -m, --mock           Enable mock mode (no camera required)\n");
    printf("  -D, --debug           Enable debug output\n");
    printf("  -l, --log-file FILE  Log to file\n");
    printf("  -s, --syslog         Enable syslog output\n");
    printf("  -c, --config FILE     Configuration file (default: /etc/smartmonitor.conf)\n");
    printf("  -h, --help           Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s                           # Use default camera\n", program_name);
    printf("  %s --mock                    # Mock mode for testing\n", program_name);
    printf("  %s --device /dev/video1 --port 8081\n", program_name);
    printf("  %s --debug --log-file /var/log/smartmonitor.log\n", program_name);
}

void parse_arguments(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"mock", no_argument, 0, 'm'},
        {"debug", no_argument, 0, 'D'},
        {"log-file", required_argument, 0, 'l'},
        {"syslog", no_argument, 0, 's'},
        {"config", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "d:p:MDl:sc:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                config.device = strdup(optarg);
                break;
            case 'p':
                config.m_port = atoi(optarg);
                break;
            case 'm':
                config.m_mock_mode = true;
                break;
            case 'D':
                config.m_debug_mode = true;
                break;
            case 'l':
                config.m_log_file = strdup(optarg);
                break;
            case 's':
                config.m_syslog_mode = true;
                break;
            case 'c':
                config.m_config_file = strdup(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Unknown option. Use -h for help.\n");
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }
}

void load_config_file() {
    FILE* file = fopen(config.m_config_file, "r");
    if (!file) {
        if (config.m_debug_mode) {
            printf("Config file %s not found, using defaults\n", config.m_config_file);
        }
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");
        
        if (key && value) {
            if (strcmp(key, "device") == 0) {
                config.device = strdup(value);
            } else if (strcmp(key, "port") == 0) {
                config.m_port = atoi(value);
            } else if (strcmp(key, "mock_mode") == 0) {
                config.m_mock_mode = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "debug") == 0) {
                config.m_debug_mode = (strcmp(value, "true") == 0);
            }
        }
    }
    fclose(file);
}

void log_message(const char* level, const char* message) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    if (config.m_log_file) {
        FILE* log = fopen(config.m_log_file, "a");
        if (log) {
            fprintf(log, "[%s] %s: %s\n", timestamp, level, message);
            fclose(log);
        }
    } else if (config.m_syslog_mode) {
        // syslog integration would go here
        printf("[%s] %s: %s\n", timestamp, level, message);
    } else {
        printf("[%s] %s: %s\n", timestamp, level, message);
    }
}

char* get_current_time(void) {
    static char time_str[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    return time_str;
}

char* calculate_uptime(time_t start_time) {
    static char uptime_str[32];
    time_t now = time(NULL);
    int seconds = difftime(now, start_time);
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    snprintf(uptime_str, sizeof(uptime_str), "%dh %dm %ds", hours, minutes, secs);
    return uptime_str;
}

static void update_system_info(uint8_t *cpu, uint8_t *mem) {
    // Расчет использования RAM через /proc/meminfo
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp) {
        long total = 0, available = 0;
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %ld kB", &total) == 1) continue;
            if (sscanf(line, "MemAvailable: %ld kB", &available) == 1) break;
        }
        fclose(fp);
        if (total > 0) {
            *mem = (uint8_t)((total - available) * 100 / total);
        }
    }

    // Расчет загрузки CPU через /proc/stat
    static long long last_total = 0, last_idle = 0;
    fp = fopen("/proc/stat", "r");
    if (fp) {
        long long user, nice, system, idle, iowait, irq, softirq, steal;
        // Читаем первую строку (суммарная статистика по всем ядрам)
        if (fscanf(fp, "cpu %lld %lld %lld %lld %lld %lld %lld %lld", 
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) >= 4) {
            
            long long current_idle = idle + iowait;
            long long current_total = user + nice + system + current_idle + irq + softirq + steal;
            
            if (last_total != 0) {
                long long total_diff = current_total - last_total;
                long long idle_diff = current_idle - last_idle;
                if (total_diff > 0) {
                    *cpu = (uint8_t)(100 * (total_diff - idle_diff) / total_diff);
                }
            }
            last_total = current_total;
            last_idle = current_idle;
        }
        fclose(fp);
    }
}

char* metrics_callback_wrapper(void) {
    // Обновляем системные показатели перед отправкой пакета
    update_system_info(&g_current_packet.cpu_usage, &g_current_packet.mem_usage);
    g_current_packet.frame_processing_latency_ms = 0.0f; // Reset or update elsewhere

    // Обновляем аптайм перед отправкой
    g_current_packet.uptime_secs = (uint32_t)difftime(time(NULL), g_start_time);
    
    // Выделяем память под копию структуры (HTTP сервер освободит её после отправки)
    monitor_packet_t* response = malloc(sizeof(monitor_packet_t));
    memcpy(response, &g_current_packet, sizeof(monitor_packet_t));
    
    return (char*)response;
}

#ifdef ENABLE_AUDIO
static audio_capture_t* audio_capture_instance = NULL; // Global audio instance
static noise_detector_t* noise_detector_instance = NULL; // Global noise detector instance

static void main_audio_toggle_callback(bool enable) {
    g_audio_capture_enabled = enable;
    if (enable) {
        if (audio_capture_instance && !audio_is_capturing(audio_capture_instance)) audio_start_capture(audio_capture_instance);
    } else {
        if (audio_capture_instance && audio_is_capturing(audio_capture_instance)) audio_stop_capture(audio_capture_instance);
    }
}
#endif

char* health_callback_wrapper(void) {
    return strdup("OK");
}

char* webrtc_callback_wrapper(void) {
    return strdup(
        "v=0\r\n"
        "o=- 0 0 IN IP4 127.0.0.1\r\n"
        "s=-\r\n"
        "t=0 0\r\n"
        "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
    );
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    parse_arguments(argc, argv);
    
    // Load configuration file
    load_config_file();
    
    // Check if port is already in use and shutdown existing instance
    if (is_port_in_use(config.m_port)) {
        printf("Port %d is already in use. Attempting to shutdown existing instance...\n", config.m_port);
        shutdown_existing_instance(config.m_port);
        
        // Check again after shutdown attempt
        if (is_port_in_use(config.m_port)) {
            fprintf(stderr, "Error: Port %d is still in use. Another instance may be running.\n", config.m_port);
            fprintf(stderr, "Please manually kill the process or use a different port.\n");
            return 1;
        }
        printf("Previous instance shutdown successfully.\n");
    }
    
    // Setup logging
    if (config.m_debug_mode) {
        log_message("DEBUG", "Smart Monitor starting in debug mode");
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGALRM, alarm_handler);
    
    log_message("INFO", "Smart Monitor starting...");
    
    g_start_time = time(NULL);
    
    // Initialize components with config
    const char* device = config.m_mock_mode ? NULL : config.device;
    v4l2_capture_t* camera = v4l2_create(device);
    if (!camera) {
        log_message("ERROR", "Failed to create camera");
    }
    
    rust_motion_detector_t* motion_detector = rust_detector_create();
    if (!motion_detector) {
        log_message("ERROR", "Failed to create motion detector");
    }
    
    http_server_t* http_server = http_server_create(config.m_port);
    if (!http_server) {
        log_message("ERROR", "Failed to create HTTP server");
    }
    
    // Initialize WebRTC server (optional)
    #ifdef ENABLE_WEBRTC
    webrtc_server_t* webrtc_server = NULL;
    (void)webrtc_server; // Suppress unused variable warning when WebRTC is disabled
    #endif
    
    if (!camera || !motion_detector || !http_server) {
        log_message("ERROR", "Failed to initialize components");
        goto cleanup;
    }
    
    // Initialize camera
    if (!v4l2_initialize(camera, DEFAULT_WIDTH, DEFAULT_HEIGHT)) {
        if (config.m_mock_mode) {
            log_message("INFO", "Mock mode enabled");
        } else {
            log_message("WARNING", "Failed to initialize camera, using mock mode");
        }
    } else {
        log_message("INFO", "Camera initialized successfully");
    }
    
    // Initialize motion detector
    if (!rust_detector_initialize(motion_detector)) {
        if (config.m_mock_mode) {
            log_message("INFO", "Mock mode: motion detector initialization skipped");
        } else {
            log_message("ERROR", "Failed to initialize motion detector");
            goto cleanup;
        }
    } else {
        log_message("INFO", "Motion detector initialized successfully");
    }
    
    // Start HTTP server
    if (!http_server_start(http_server)) {
        log_message("ERROR", "Failed to start HTTP server");
        goto cleanup;
    }
    
    char port_msg[64];
    snprintf(port_msg, sizeof(port_msg), "HTTP server started on port %d", config.m_port);
    log_message("INFO", port_msg);
    
    // Initialize WebRTC server
    #ifdef ENABLE_WEBRTC
    if (webrtc_server && webrtc_server_initialize(webrtc_server, device)) {
        log_message("INFO", "WebRTC server initialized");
    }
    #endif
    
    // Initialize optional components
    #ifdef ENABLE_AUDIO
    audio_capture_t* audio = audio_create_mock();
    if (audio && audio_initialize(audio, 44100, 1)) {
        audio_start_capture(audio);
        log_message("INFO", "Audio capture initialized");
    }
    
    noise_detector_t* noise_detector = noise_detector_create(44100);
    if (noise_detector && noise_detector_initialize(noise_detector)) {
        log_message("INFO", "Noise detection initialized");
    }
    #endif
    
    #ifdef ENABLE_SENSORS
    // I2C sensor
    i2c_sensor_t* i2c_sensor = i2c_create_mock();
    if (i2c_sensor && i2c_initialize(i2c_sensor)) {
        log_message("INFO", "I2C sensor initialized");
    }
    
    // SPI device (IMU)
    spi_device_t* spi_device = spi_create_mock();
    if (spi_device && spi_initialize(spi_device, 0, 1000000)) {
        log_message("INFO", "SPI IMU device initialized");
    }
    
    // UART interface
    uart_interface_t* uart = uart_create_mock();
    if (uart && uart_initialize(uart, 115200)) {
        log_message("INFO", "UART interface initialized");
    }
    
    // ESP32 device
    esp32_device_t* esp32 = esp32_create_mock();
    if (esp32 && esp32_initialize(esp32, 115200)) {
        if (esp32_connect(esp32)) {
            log_message("INFO", "ESP32 device connected");
        }
    }
    #endif
    
    log_message("INFO", "All components initialized");
    
    // Start camera capture
    if (v4l2_is_initialized(camera)) {
        if (!v4l2_start_capture(camera)) {
            fprintf(stderr, "Failed to start camera capture\n");
        } else {
            metrics_data_t* metrics = http_server_get_metrics(http_server);
            if (metrics) {
                metrics->m_camera_active = true;
                g_current_packet.camera_active = 1;
            }
            printf("Camera capture started\n");
        }
    }
    
    // Main processing loop
    uint8_t* prev_frame = NULL;
    size_t prev_frame_size __attribute__((unused)) = 0;
    
    while (running) {
        uint8_t* current_frame = NULL;
        size_t current_frame_size = 0;
        
        if (v4l2_is_initialized(camera)) {
            current_frame = v4l2_read_frame_raw(camera, &current_frame_size);
        } else {
            // Mock frame data with simulated motion
            current_frame_size = 640 * 480 * 2; // YUYV format
            current_frame = malloc(current_frame_size);
            if (current_frame) {
                static int frame_counter = 0;
                memset(current_frame, 0x80, current_frame_size); // Gray background
                
                // Add moving object that creates motion
                int object_x = (frame_counter * 10) % (640 - 100) + 50;
                int object_y = (frame_counter * 5) % (480 - 100) + 50;
                int object_size = 80;
                
                for (int y = object_y; y < object_y + object_size && y < 480; y++) {
                    for (int x = object_x; x < object_x + object_size && x < 640; x++) {
                        int pixel_idx = (y * 640 + x) * 2;
                        if ((size_t)pixel_idx < current_frame_size - 1) {
                            // Bright white object for motion detection
                            current_frame[pixel_idx] = 0xFF; // Y component
                            current_frame[pixel_idx + 1] = 0x80; // U/V component
                        }
                    }
                }
                frame_counter++;
            }
        }
        
        if (current_frame && prev_frame) {
            // Convert to grayscale for motion detection (YUYV -> Y)
            size_t gray_size = current_frame_size / 2;
            uint8_t* gray_current = malloc(gray_size);
            uint8_t* gray_prev = malloc(gray_size);
            
            if (gray_current && gray_prev) {
                for (size_t i = 0, j = 0; i < current_frame_size && j < gray_size; i += 2, ++j) {
                    gray_current[j] = current_frame[i]; // Y component
                    gray_prev[j] = prev_frame[i];     // Y component
                }
                
                // Detect motion using Rust module
                if (gray_prev && gray_current && motion_detector) {
                    // Validate frame data
                    if (gray_size > 0 && gray_size <= 1000000) { // 1MB limit
                        // Ensure detector is initialized
                        if (!rust_detector_initialize(motion_detector)) {
                            log_message("ERROR", "Failed to initialize motion detector before detection");
                        } else {
                            // Add signal handler for safety
                            signal(SIGSEGV, signal_handler);
                            
                            struct timespec start_time_ns, end_time_ns;
                            long long elapsed_ns;

                            clock_gettime(CLOCK_MONOTONIC, &start_time_ns);
                            motion_result_t result = rust_detector_detect_motion_advanced(
                                motion_detector,
                                gray_prev,
                                gray_current,
                                640,
                                480,
                                20
                            );
                            
                            // Restore default signal handler
                            clock_gettime(CLOCK_MONOTONIC, &end_time_ns);
                            elapsed_ns = (end_time_ns.tv_sec - start_time_ns.tv_sec) * 1000000000LL +
                                         (end_time_ns.tv_nsec - start_time_ns.tv_nsec);
                            g_current_packet.frame_processing_latency_ms = (float)elapsed_ns / 1000000.0f;
                            signal(SIGSEGV, SIG_DFL);
                            
                            if (result.motion_detected) {
                                metrics_data_t* metrics = http_server_get_metrics(http_server);
                                g_current_packet.motion_events++;
                                g_current_packet.motion_level = result.motion_level;
                                if (metrics) {
                                    metrics->m_motion_events++;
                                    metrics->m_motion_level = result.motion_level;
                                    strncpy(metrics->m_last_motion_time, get_current_time(), 
                                           sizeof(metrics->m_last_motion_time) - 1);
                                }
                            }
                            
                            #ifdef ENABLE_AUDIO
                            // Audio analysis
                            if (audio_capture_instance && noise_detector_instance && g_audio_capture_enabled) {
                                uint8_t audio_samples[4096];
                                int samples_read = audio_read_samples(audio_capture_instance, audio_samples, sizeof(audio_samples));
                                if (samples_read > 0) {
                                    noise_metrics_t noise_metrics = noise_detector_analyze(noise_detector, audio_samples, samples_read);
                                    
                                    g_current_packet.audio_level = noise_metrics.m_noise_level;
                                    
                                    // Log significant events with throttling
                                    static uint32_t last_alert_time = 0;
                                    uint32_t current_time = time(NULL);
                                    
                                    if (noise_metrics.m_baby_crying_detected && (current_time - last_alert_time) > 30) {
                                        log_message("ALERT", "Baby crying detected!");
                                        last_alert_time = current_time;
                                        g_current_packet.audio_alert = 1;
                                    }
                                    if (noise_metrics.m_screaming_detected && (current_time - last_alert_time) > 30) {
                                        log_message("ALERT", "Screaming detected!");
                                        last_alert_time = current_time;
                                    }
                                }
                            }
                            #endif
                            
                            #ifdef ENABLE_SENSORS
                            // Sensor data collection
                            static uint32_t sensor_counter = 0;
                            sensor_counter++;
                            
                            // I2C sensor reading
                            if (i2c_sensor && sensor_counter % 100 == 0) {
                                sensor_data_t sensor_data = i2c_read_sensor_data(i2c_sensor);
                                g_current_packet.temp = sensor_data.temperature;
                                g_current_packet.humidity = sensor_data.humidity;
                                g_current_packet.light = sensor_data.light_level;
                                g_current_packet.motion_detected = sensor_data.motion_detected;
                                
                                char sensor_msg[128];
                                snprintf(sensor_msg, sizeof(sensor_msg), 
                                        "I2C: T=%.1f°C H=%.1f%% L=%d M=%s",
                                        sensor_data.temperature, sensor_data.humidity,
                                        sensor_data.light_level, sensor_data.motion_detected ? "YES" : "NO");
                            }
                            
                            // SPI IMU reading
                            if (spi_device && sensor_counter % 50 == 0) {
                                imu_data_t imu_data = spi_read_imu_data(spi_device);
                                g_current_packet.acc_x = imu_data.m_accelerometer_x;
                                g_current_packet.acc_y = imu_data.m_accelerometer_y;
                                g_current_packet.acc_z = imu_data.m_accelerometer_z;
                                
                                char imu_msg[128];
                                // log_message("SENSOR", imu_msg);
                            }
                            
                            // ESP32 device reading
                            if (esp32 && sensor_counter % 200 == 0) {
                                esp32_sensor_data_t esp32_data = esp32_read_sensors(esp32);
                                char esp32_msg[128];
                                snprintf(esp32_msg, sizeof(esp32_msg),
                                        "ESP32: T=%.1f°C H=%.1f%% BAT=%d%% L=%d",
                                        esp32_data.temperature, esp32_data.humidity,
                                        esp32_data.battery_level, esp32_data.light_level);
                                
                                g_current_packet.esp32_temp = esp32_data.temperature;
                                g_current_packet.esp32_hum = esp32_data.humidity;
                                g_current_packet.battery = esp32_data.battery_level;
                                
                                
                                // Send data via UART
                                if (uart) {
                                    uart_log_message(uart, "DATA", esp32_msg);
                                }
                            }
                            
                            // UART command processing
                            if (uart && sensor_counter % 300 == 0) {
                                uart_process_commands(uart);
                            }
                            #endif
                        }
                    } else {
                        fprintf(stderr, "Invalid frame size: %zu\n", gray_size);
                    }
                }
                
                metrics_data_t* metrics = http_server_get_metrics(http_server);
                if (metrics) {
                    g_current_packet.frames_processed++;
                    metrics->m_frames_processed++;
                }
            }
            
            free(gray_current);
            free(gray_prev);
        }
        
        // Clean up previous frame
        if (prev_frame) {
            free(prev_frame);
        }
        
        prev_frame = current_frame;
        prev_frame_size = current_frame_size;
        
        // Update uptime
        metrics_data_t* metrics = http_server_get_metrics(http_server);
        if (metrics) {
            strncpy(metrics->m_uptime, calculate_uptime(g_start_time), 
                   sizeof(metrics->m_uptime) - 1);
        }
        
        // Sleep to control frame rate with signal checking
        for (int i = 0; i < 33 && running; i++) {
            usleep(1000); // Sleep in 1ms chunks to allow faster signal response
        }
    }
    
    // Cleanup
    if (prev_frame) {
        free(prev_frame);
    }
    
cleanup:
    printf("\nShutting down Smart Monitor...\n");
    
    if (camera) {
        v4l2_destroy(camera);
    }
#ifdef ENABLE_AUDIO
    if (audio_capture_instance) audio_destroy(audio_capture_instance);
    if (noise_detector_instance) noise_detector_destroy(noise_detector_instance);
#endif
    if (motion_detector) {
        rust_detector_destroy(motion_detector);
    }
    
    if (http_server) {
        http_server_destroy(http_server);
    }
    
    return 0;
}
