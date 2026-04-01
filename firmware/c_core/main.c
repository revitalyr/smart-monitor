#include "video/v4l2_capture.h"
#include "video/gstreamer_pipeline.h"
#include "ffi/rust_bridge.h"
#include "net/http_server.h"
#include "net/webrtc_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
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

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Smart Monitor starting...\n");
    
    time_t start_time = time(NULL);
    
    // Initialize components
    v4l2_capture_t* camera = v4l2_create("/dev/video0");
    rust_motion_detector_t* motion_detector = rust_detector_create();
    http_server_t* http_server = http_server_create(8080);
    webrtc_server_t* webrtc_server = webrtc_server_create("/dev/video0");
    
    if (!camera || !motion_detector || !http_server || !webrtc_server) {
        fprintf(stderr, "Failed to initialize components\n");
        goto cleanup;
    }
    
    // Initialize camera
    if (!v4l2_initialize(camera, 640, 480)) {
        printf("Failed to initialize camera, using mock mode\n");
    } else {
        printf("Camera initialized successfully\n");
    }
    
    // Initialize motion detector
    if (!rust_detector_initialize(motion_detector)) {
        fprintf(stderr, "Failed to initialize motion detector\n");
        goto cleanup;
    }
    
    // Start HTTP server
    if (!http_server_start(http_server)) {
        fprintf(stderr, "Failed to start HTTP server\n");
        goto cleanup;
    }
    
    printf("HTTP server started on port 8080\n");
    
    // Initialize WebRTC server
    if (webrtc_server_initialize(webrtc_server, "/dev/video0")) {
        printf("WebRTC server initialized\n");
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
