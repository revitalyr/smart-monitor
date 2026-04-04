/**
 * @file main.c
 * @brief Smart Monitor Main Application
 * 
 * This file implements the main application logic using the new architecture:
 * - Data Agent: Simulates and processes sensor data
 * - Protocol Server: Handles binary protocol communication
 * - HTTP Server: Provides web interface (no simulation)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>

#include "agent/data_agent.h"
#include "../../web/protocol_server.h"
#include "../../web/http_server.h"
#include "common/smart_monitor_constants.h"

// Forward declarations for data agent JSON generators
extern char* generate_sensor_json(void);
extern char* generate_audio_json(void);
extern char* generate_system_json(void);

// Global instances
static data_agent_t* g_data_agent = NULL;
static protocol_server_t* g_protocol_server = NULL;
static http_server_t* g_http_server = NULL;
static volatile bool g_running = true;

// Default configuration
static const char* DEFAULT_DEVICE = "/dev/video0";
static const int DEFAULT_PORT = 8080;
static const int DEFAULT_PROTOCOL_PORT = 8082;

// Signal handlers
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    g_running = false;
}

// Print usage information
static void print_usage(const char* program_name) {
    printf("Smart Monitor - IoT Monitoring System\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -d, --device PATH     Camera device path (default: %s)\n", DEFAULT_DEVICE);
    printf("  -p, --port PORT      HTTP server port (default: %d)\n", DEFAULT_PORT);
    printf("  -P, --protocol-port PORT  Protocol server port (default: %d)\n", DEFAULT_PROTOCOL_PORT);
    printf("  -s, --simulation     Enable simulation mode\n");
    printf("  -c, --camera         Enable camera processing\n");
    printf("  -a, --audio          Enable audio processing\n");
    printf("  -S, --sensors        Enable sensor processing\n");
    printf("  -h, --help          Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s --simulation --camera --audio --sensors\n", program_name);
    printf("  %s --device /dev/video0 --port 8080\n", program_name);
}

// HTTP API handlers - these bridge to the data agent
static char* http_sensor_handler(void) {
    if (g_data_agent) {
        return data_agent_get_sensor_json(g_data_agent);
    }
    return strdup("{\"error\":\"data_agent_not_available\"}");
}

static char* http_audio_handler(void) {
    if (g_data_agent) {
        return data_agent_get_audio_json(g_data_agent);
    }
    return strdup("{\"error\":\"data_agent_not_available\"}");
}

static char* http_video_handler(void) {
    if (g_data_agent) {
        return data_agent_get_video_json(g_data_agent);
    }
    return strdup("{\"error\":\"data_agent_not_available\"}");
}

static char* http_system_handler(void) {
    if (g_data_agent) {
        return data_agent_get_system_json(g_data_agent);
    }
    return strdup("{\"error\":\"data_agent_not_available\"}");
}

int main(int argc, char* argv[]) {
    // Default configuration
    const char* device = DEFAULT_DEVICE;
    int http_port = DEFAULT_PORT;
    int protocol_port = DEFAULT_PROTOCOL_PORT;
    bool enable_simulation = false;
    bool enable_camera = false;
    bool enable_audio = false;
    bool enable_sensors = false;
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"protocol-port", required_argument, 0, 'P'},
        {"simulation", no_argument, 0, 's'},
        {"camera", no_argument, 0, 'c'},
        {"audio", no_argument, 0, 'a'},
        {"sensors", no_argument, 0, 'S'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "d:p:P:scaSh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                device = optarg;
                break;
            case 'p':
                http_port = atoi(optarg);
                break;
            case 'P':
                protocol_port = atoi(optarg);
                break;
            case 's':
                enable_simulation = true;
                break;
            case 'c':
                enable_camera = true;
                break;
            case 'a':
                enable_audio = true;
                break;
            case 'S':
                enable_sensors = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case '?':
                print_usage(argv[0]);
                return 1;
            default:
                break;
        }
    }
    
    // If no specific features enabled, enable all in simulation mode
    if (!enable_camera && !enable_audio && !enable_sensors) {
        enable_simulation = true;
        enable_camera = true;
        enable_audio = true;
        enable_sensors = true;
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Smart Monitor Starting...\n");
    printf("Configuration:\n");
    printf("  Device: %s\n", device);
    printf("  HTTP Port: %d\n", http_port);
    printf("  Protocol Port: %d\n", protocol_port);
    printf("  Simulation: %s\n", enable_simulation ? "Yes" : "No");
    printf("  Camera: %s\n", enable_camera ? "Yes" : "No");
    printf("  Audio: %s\n", enable_audio ? "Yes" : "No");
    printf("  Sensors: %s\n", enable_sensors ? "Yes" : "No");
    printf("\n");
    
    // Initialize data agent
    agent_config_t agent_config = {
        .enable_camera = enable_camera,
        .enable_audio = enable_audio,
        .enable_sensors = enable_sensors,
        .enable_simulation = enable_simulation,
        .update_interval_ms = 100, // 10 Hz update rate
        .video_source = (char*)device,
        .motion_threshold = 0.3f,
        .audio_threshold = 0.5f
    };
    
    g_data_agent = data_agent_create(&agent_config);
    if (!g_data_agent) {
        fprintf(stderr, "Failed to create data agent\n");
        return 1;
    }
    
    printf("Data agent created successfully\n");
    
    // Initialize protocol server
    server_config_t protocol_config = {
        .port = protocol_port,
        .max_clients = 10,
        .enable_websocket = false,
        .heartbeat_interval_ms = 5000,
        .timeout_ms = 30000
    };
    
    g_protocol_server = protocol_server_create(&protocol_config);
    if (!g_protocol_server) {
        fprintf(stderr, "Failed to create protocol server\n");
        data_agent_destroy(g_data_agent);
        return 1;
    }
    
    // Connect agent to protocol server
    protocol_server_set_agent(g_protocol_server, g_data_agent);
    
    printf("Protocol server created successfully\n");
    
    // Initialize HTTP server
    g_http_server = http_server_create(http_port);
    if (!g_http_server) {
        fprintf(stderr, "Failed to create HTTP server\n");
        protocol_server_destroy(g_protocol_server);
        data_agent_destroy(g_data_agent);
        return 1;
    }
    
    // Set HTTP handlers
    http_server_set_sensor_data_callback(g_http_server, http_sensor_handler);
    http_server_set_audio_data_callback(g_http_server, http_audio_handler);
    http_server_set_video_data_callback(g_http_server, http_video_handler);
    http_server_set_system_data_callback(g_http_server, http_system_handler);
    
    // Initialize data agent HTTP callbacks
    data_agent_set_http_callbacks(g_data_agent, 
                                  generate_sensor_json, 
                                  generate_audio_json, 
                                  generate_system_json);
    
    // Set video callback using the function
    data_agent_set_video_callback(g_data_agent, generate_video_json);
    
    printf("HTTP server created successfully\n");
    
    // Start all services
    if (!data_agent_start(g_data_agent)) {
        fprintf(stderr, "Failed to start data agent\n");
        goto cleanup;
    }
    printf("Data agent started\n");
    
    if (!protocol_server_start(g_protocol_server)) {
        fprintf(stderr, "Failed to start protocol server\n");
        goto cleanup;
    }
    printf("Protocol server started on port %d\n", protocol_port);
    
    if (!http_server_start(g_http_server)) {
        fprintf(stderr, "Failed to start HTTP server\n");
        goto cleanup;
    }
    printf("HTTP server started on port %d\n", http_port);
    printf("Web interface: http://localhost:%d\n", http_port);
    printf("Protocol server: localhost:%d\n", protocol_port);
    printf("\nPress Ctrl+C to stop...\n");
    
    // Main loop
    while (g_running) {
        sleep(1);
        
        // Print statistics every 10 seconds
        static int stats_counter = 0;
        if (++stats_counter >= 10) {
            stats_counter = 0;
            
            // Data agent statistics
            agent_stats_t agent_stats = data_agent_get_stats(g_data_agent);
            printf("Agent Stats - Frames: %u, Motion: %u, Audio: %u, Sensors: %u\n",
                   agent_stats.frames_processed, agent_stats.motion_events,
                   agent_stats.audio_events, agent_stats.sensor_updates);
            
            // Protocol server statistics
            server_stats_t server_stats = protocol_server_get_stats(g_protocol_server);
            printf("Protocol Stats - Clients: %d, Messages: %lu sent, %lu received\n",
                   server_stats.active_connections, server_stats.messages_sent, server_stats.messages_received);
        }
    }
    
cleanup:
    printf("\nShutting down...\n");
    
    // Stop services
    if (g_http_server) {
        http_server_stop(g_http_server);
        http_server_destroy(g_http_server);
        printf("HTTP server stopped\n");
    }
    
    if (g_protocol_server) {
        protocol_server_stop(g_protocol_server);
        protocol_server_destroy(g_protocol_server);
        printf("Protocol server stopped\n");
    }
    
    if (g_data_agent) {
        data_agent_stop(g_data_agent);
        data_agent_destroy(g_data_agent);
        printf("Data agent stopped\n");
    }
    
    printf("Smart Monitor shutdown complete\n");
    return 0;
}
