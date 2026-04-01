#include "video/v4l2_capture.h"
#include "video/gstreamer_pipeline.h"
#include "net/http_server.h"
#include "net/webrtc_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>

#include "video/v4l2_capture.h"
#include "net/http_server.h"
#include "net/webrtc_server.h"
#include "ffi/rust_bridge.h"

#define DEFAULT_PORT 8080
#define DEFAULT_DEVICE "/dev/video0"
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

typedef struct {
    char* device;
    int port;
    bool mock_mode;
    bool debug_mode;
    char* log_file;
    bool syslog_mode;
    char* config_file;
} config_t;

static volatile bool running = true;
static config_t config = {
    .device = DEFAULT_DEVICE,
    .port = DEFAULT_PORT,
    .mock_mode = false,
    .debug_mode = false,
    .log_file = NULL,
    .syslog_mode = false,
    .config_file = "/etc/smartmonitor.conf"
};

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down Smart Monitor...\n");
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
                config.port = atoi(optarg);
                break;
            case 'm':
                config.mock_mode = true;
                break;
            case 'D':
                config.debug_mode = true;
                break;
            case 'l':
                config.log_file = strdup(optarg);
                break;
            case 's':
                config.syslog_mode = true;
                break;
            case 'c':
                config.config_file = strdup(optarg);
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
    FILE* file = fopen(config.config_file, "r");
    if (!file) {
        if (config.debug_mode) {
            printf("Config file %s not found, using defaults\n", config.config_file);
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
                config.port = atoi(value);
            } else if (strcmp(key, "mock_mode") == 0) {
                config.mock_mode = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "debug") == 0) {
                config.debug_mode = (strcmp(value, "true") == 0);
            }
        }
    }
    fclose(file);
}

void log_message(const char* level, const char* message) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    if (config.log_file) {
        FILE* log = fopen(config.log_file, "a");
        if (log) {
            fprintf(log, "[%s] %s: %s\n", timestamp, level, message);
            fclose(log);
        }
    } else if (config.syslog_mode) {
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

char* metrics_callback_wrapper(void) {
    // This would be called from HTTP server
    // For now, return a simple JSON
    return strdup("{\"motion_events\":0,\"motion_level\":0.00,\"frames_processed\":0,\"camera_active\":false,\"last_motion_time\":\"\",\"uptime\":\"0s\"}");
}

char* health_callback_wrapper(void) {
    return strdup("{\"status\":\"ok\",\"service\":\"smart-monitor\",\"version\":\"1.0.0\"}");
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
    
    // Setup logging
    if (config.debug_mode) {
        log_message("DEBUG", "Smart Monitor starting in debug mode");
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    log_message("INFO", "Smart Monitor starting...");
    
    time_t start_time = time(NULL);
    
    // Initialize components with config
    const char* device = config.mock_mode ? NULL : config.device;
    v4l2_capture_t* camera = v4l2_create(device);
    rust_motion_detector_t* motion_detector = rust_detector_create();
    http_server_t* http_server = http_server_create(config.port);
    webrtc_server_t* webrtc_server = webrtc_server_create(device);
    
    if (!camera || !motion_detector || !http_server || !webrtc_server) {
        log_message("ERROR", "Failed to initialize components");
        goto cleanup;
    }
    
    // Initialize camera
    if (!v4l2_initialize(camera, DEFAULT_WIDTH, DEFAULT_HEIGHT)) {
        if (config.mock_mode) {
            log_message("INFO", "Mock mode enabled");
        } else {
            log_message("WARNING", "Failed to initialize camera, using mock mode");
        }
    } else {
        log_message("INFO", "Camera initialized successfully");
    }
    
    // Initialize motion detector
    if (!rust_detector_initialize(motion_detector)) {
        log_message("ERROR", "Failed to initialize motion detector");
        goto cleanup;
    }
    
    // Start HTTP server
    if (!http_server_start(http_server)) {
        log_message("ERROR", "Failed to start HTTP server");
        goto cleanup;
    }
    
    char port_msg[64];
    snprintf(port_msg, sizeof(port_msg), "HTTP server started on port %d", config.port);
    log_message("INFO", port_msg);
    
    // Initialize WebRTC server
    if (webrtc_server_initialize(webrtc_server, device)) {
        log_message("INFO", "WebRTC server initialized");
    }
    
    // Set HTTP server callbacks
    http_server_set_metrics_callback(http_server, metrics_callback_wrapper);
    http_server_set_health_callback(http_server, health_callback_wrapper);
    http_server_set_webrtc_callback(http_server, webrtc_callback_wrapper);
    
    // Start camera capture
    if (v4l2_is_initialized(camera)) {
        if (!v4l2_start_capture(camera)) {
            fprintf(stderr, "Failed to start camera capture\n");
        } else {
            metrics_data_t* metrics = http_server_get_metrics(http_server);
            if (metrics) {
                metrics->camera_active = true;
            }
            printf("Camera capture started\n");
        }
    }
    
    // Main processing loop
    uint8_t* prev_frame = NULL;
    size_t prev_frame_size = 0;
    
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
                        if (pixel_idx < current_frame_size - 1) {
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
                            fprintf(stderr, "Failed to initialize motion detector\n");
                        } else {
                            // Add signal handler for safety
                            signal(SIGSEGV, signal_handler);
                            
                            motion_result_t result = rust_detector_detect_motion_advanced(
                                motion_detector,
                                gray_prev,
                                gray_current,
                                640,
                                480,
                                20
                            );
                            
                            // Restore default signal handler
                            signal(SIGSEGV, SIG_DFL);
                            
                            if (result.motion_detected) {
                                metrics_data_t* metrics = http_server_get_metrics(http_server);
                                if (metrics) {
                                    metrics->motion_events++;
                                    metrics->motion_level = result.motion_level;
                                    strncpy(metrics->last_motion_time, get_current_time(), 
                                           sizeof(metrics->last_motion_time) - 1);
                                }
                                
                                printf("Motion detected! Level: %.2f, Changed pixels: %u\n", 
                                       result.motion_level, result.changed_pixels);
                            }
                        }
                    } else {
                        fprintf(stderr, "Invalid frame size: %zu\n", gray_size);
                    }
                }
                
                metrics_data_t* metrics = http_server_get_metrics(http_server);
                if (metrics) {
                    metrics->frames_processed++;
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
            strncpy(metrics->uptime, calculate_uptime(start_time), 
                   sizeof(metrics->uptime) - 1);
        }
        
        // Sleep to control frame rate
        usleep(33000); // ~30 FPS
    }
    
    // Cleanup
    if (prev_frame) {
        free(prev_frame);
    }
    
cleanup:
    printf("\nShutting down Smart Monitor...\n");
    
    if (camera) {
        v4l2_stop_capture(camera);
        v4l2_destroy(camera);
    }
    
    if (motion_detector) {
        rust_detector_destroy(motion_detector);
    }
    
    if (http_server) {
        http_server_stop(http_server);
        http_server_destroy(http_server);
    }
    
    if (webrtc_server) {
        webrtc_server_stop(webrtc_server);
        webrtc_server_destroy(webrtc_server);
    }
    
    printf("Smart Monitor stopped\n");
    
    return 0;
}
