#include "protocol_server.h"
#include "../../core/common/smart_monitor_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_CLIENTS 32

struct ProtocolServer {
    ServerConfig config;
    DataAgent* agent;
    
    FileDescriptor server_fd;
    pthread_t accept_thread;
    volatile bool running;
    
    ClientConnection clients[MAX_CLIENTS];
    int client_count;
    
    pthread_mutex_t clients_mutex;
    ServerStats stats;
};

// Helper function to get current timestamp
static TimestampMs get_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TimestampMs)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

// Send protocol message to client
static bool send_protocol_message(FileDescriptor client_fd, const ProtocolHeader* header, 
                              const void* payload, ByteCount payload_size) {
    // Create complete message
    ByteCount total_size = sizeof(ProtocolHeader) + payload_size;
    uint8_t* message = malloc(total_size);
    if (!message) return false;
    
    // Copy header and calculate checksum
    ProtocolHeader header_copy = *header;
    if (payload_size > 0) {
        memcpy(message + sizeof(ProtocolHeader), payload, payload_size);
    }
    header_copy.checksum = protocol_calculate_crc(message, total_size);
    
    // Copy header with checksum
    memcpy(message, &header_copy, sizeof(ProtocolHeader));
    
    // Send message
    ssize_t sent = send(client_fd, message, total_size, MSG_NOSIGNAL);
    free(message);
    
    return sent == (ssize_t)total_size;
}

// Handle incoming message from client
static void handle_client_message(ProtocolServer* server, ClientConnection* client) {
    if (client->buffer_len < sizeof(ProtocolHeader)) {
        return; // Not enough data for header
    }
    
    ProtocolHeader* header = (ProtocolHeader*)client->buffer;
    
    // Validate header
    if (!protocol_validate_header(header)) {
        printf("Invalid protocol header received from client\n");
        return;
    }
    
    // Check if we have complete message
    ByteCount expected_size = sizeof(ProtocolHeader) + header->payload_length;
    if (client->buffer_len < expected_size) {
        return; // Incomplete message
    }
    
    void* payload = client->buffer + sizeof(ProtocolHeader);
    
    // Process message based on type
    switch (header->type) {
        case MSG_TYPE_COMMAND: {
            if (header->payload_length >= sizeof(CommandPayload)) {
                CommandPayload* cmd = (CommandPayload*)payload;
                
                // Handle commands
                ResponsePayload response = {0};
                response.status = 0; // Success
                
                switch (cmd->command) {
                    case CMD_START_CAPTURE:
                        // TODO: Start capture
                        printf("Start capture command received\n");
                        break;
                        
                    case CMD_STOP_CAPTURE:
                        // TODO: Stop capture
                        printf("Stop capture command received\n");
                        break;
                        
                    case CMD_GET_STATUS:
                        // TODO: Send status
                        printf("Get status command received\n");
                        break;
                        
                    case CMD_SET_THRESHOLD:
                        // TODO: Set threshold
                        printf("Set threshold command: %f\n", cmd->value / 1000.0f);
                        if (server->agent) {
                            data_agent_set_motion_threshold(server->agent, cmd->value / 1000.0f);
                        }
                        break;
                        
                    default:
                        response.status = 1; // Error
                        response.error_code = 1; // Unknown command
                        break;
                }
                
                // Send response
                ProtocolHeader response_header;
                protocol_create_header(&response_header, MSG_TYPE_RESPONSE, 
                                    server->stats.messages_sent + 1, get_timestamp_ms(), 
                                    sizeof(ResponsePayload));
                
                send_protocol_message(client->socket_fd, &response_header, 
                                  &response, sizeof(response));
            }
            break;
        }
        
        case MSG_TYPE_HEARTBEAT: {
            // Send heartbeat response
            ProtocolHeader response_header;
            protocol_create_header(&response_header, MSG_TYPE_HEARTBEAT, 
                                server->stats.messages_sent + 1, get_timestamp_ms(), 0);
            
            send_protocol_message(client->socket_fd, &response_header, NULL, 0);
            break;
        }
        
        default:
            printf("Unknown message type: %d\n", header->type);
            break;
    }
    
    // Remove processed message from buffer
    ByteCount remaining = client->buffer_len - expected_size;
    if (remaining > 0) {
        memmove(client->buffer, client->buffer + expected_size, remaining);
    }
    client->buffer_len = remaining;
    
    server->stats.messages_received++;
    client->last_activity = get_timestamp_ms();
}

// Accept new connections
static void* accept_thread_func(void* arg) {
    ProtocolServer* server = (ProtocolServer*)arg;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        FileDescriptor client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                perror("accept failed");
            }
            continue;
        }
        
        // Set socket to non-blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        
        pthread_mutex_lock(&server->clients_mutex);
        
        // Find free slot
        if (server->client_count < MAX_CLIENTS) {
            int slot = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server->clients[i].socket_fd == -1) {
                    slot = i;
                    break;
                }
            }
            
            // Initialize client
            ClientConnection* client = &server->clients[slot];
            client->socket_fd = client_fd;
            client->address = client_addr;
            client->last_activity = get_timestamp_ms();
            client->sequence_in = 0;
            client->sequence_out = 0;
            client->authenticated = true; // TODO: implement authentication
            client->buffer_len = 0;
            
            server->client_count++;
            server->stats.total_connections++;
            
            printf("Client connected: %s:%d (total: %d)\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), 
                   server->client_count);
        } else {
            printf("Max clients reached, rejecting connection\n");
            close(client_fd);
        }
        
        pthread_mutex_unlock(&server->clients_mutex);
    }
    
    return NULL;
}

// Agent callbacks
static void sensor_data_callback(const SensorData* data, void* user_data) {
    ProtocolServer* server = (ProtocolServer*)user_data;
    
    ProtocolHeader header;
    protocol_create_header(&header, MSG_TYPE_SENSOR_DATA, 
                        server->stats.messages_sent + 1, get_timestamp_ms(), 
                        sizeof(SensorData));
    
    protocol_server_broadcast_message(server, &header, data, sizeof(SensorData));
}

static void audio_data_callback(const AudioData* data, void* user_data) {
    ProtocolServer* server = (ProtocolServer*)user_data;
    
    ProtocolHeader header;
    protocol_create_header(&header, MSG_TYPE_AUDIO_DATA, 
                        server->stats.messages_sent + 1, get_timestamp_ms(), 
                        sizeof(AudioData));
    
    protocol_server_broadcast_message(server, &header, data, sizeof(AudioData));
}

static void motion_alert_callback(const MotionAlert* alert, void* user_data) {
    ProtocolServer* server = (ProtocolServer*)user_data;
    
    ProtocolHeader header;
    protocol_create_header(&header, MSG_TYPE_MOTION_ALERT, 
                        server->stats.messages_sent + 1, get_timestamp_ms(), 
                        sizeof(MotionAlert));
    
    protocol_server_broadcast_message(server, &header, alert, sizeof(MotionAlert));
}

static void system_status_callback(const SystemStatus* status, void* user_data) {
    ProtocolServer* server = (ProtocolServer*)user_data;
    
    ProtocolHeader header;
    protocol_create_header(&header, MSG_TYPE_SYSTEM_STATUS, 
                        server->stats.messages_sent + 1, get_timestamp_ms(), 
                        sizeof(SystemStatus));
    
    protocol_server_broadcast_message(server, &header, status, sizeof(SystemStatus));
}

// Public API implementation
ProtocolServer* protocol_server_create(const ServerConfig* config) {
    ProtocolServer* server = calloc(1, sizeof(ProtocolServer));
    if (!server) return NULL;
    
    memcpy(&server->config, config, sizeof(ServerConfig));
    server->stats.start_time = get_timestamp_ms();
    
    // Initialize clients array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i].socket_fd = -1;
    }
    
    // Create server socket
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("socket creation failed");
        free(server);
        return NULL;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->port);
    
    if (bind(server->server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server->server_fd);
        free(server);
        return NULL;
    }
    
    // Initialize mutex
    pthread_mutex_init(&server->clients_mutex, NULL);
    
    return server;
}

void protocol_server_destroy(ProtocolServer* server) {
    if (!server) return;
    
    protocol_server_stop(server);
    
    if (server->server_fd >= 0) {
        close(server->server_fd);
    }
    
    pthread_mutex_destroy(&server->clients_mutex);
    free(server);
}

bool protocol_server_start(ProtocolServer* server) {
    if (!server || server->running) return false;
    
    // Start listening
    if (listen(server->server_fd, server->config.max_clients) < 0) {
        perror("listen failed");
        return false;
    }
    
    server->running = true;
    
    // Start accept thread
    if (pthread_create(&server->accept_thread, NULL, accept_thread_func, server) != 0) {
        perror("pthread_create failed");
        server->running = false;
        return false;
    }
    
    printf("Protocol server started on port %d\n", server->config.port);
    return true;
}

void protocol_server_stop(ProtocolServer* server) {
    if (!server || !server->running) return;
    
    server->running = false;
    
    // Close all client connections
    pthread_mutex_lock(&server->clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].socket_fd >= 0) {
            close(server->clients[i].socket_fd);
            server->clients[i].socket_fd = -1;
        }
    }
    server->client_count = 0;
    pthread_mutex_unlock(&server->clients_mutex);
    
    // Wait for accept thread
    pthread_join(server->accept_thread, NULL);
    
    printf("Protocol server stopped\n");
}

bool protocol_server_is_running(const ProtocolServer* server) {
    return server ? server->running : false;
}

void protocol_server_set_agent(ProtocolServer* server, DataAgent* agent) {
    if (server && agent) {
        server->agent = agent;
        
        // Register callbacks
        data_agent_set_sensor_callback(agent, sensor_data_callback, server);
        data_agent_set_audio_callback(agent, audio_data_callback, server);
        data_agent_set_motion_callback(agent, motion_alert_callback, server);
        data_agent_set_status_callback(agent, system_status_callback, server);
    }
}

int protocol_server_get_client_count(const ProtocolServer* server) {
    return server ? server->client_count : 0;
}

void protocol_server_broadcast_message(ProtocolServer* server, const ProtocolHeader* header, 
                                  const void* payload, ByteCount payload_size) {
    if (!server || !server->running) return;
    
    pthread_mutex_lock(&server->clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].socket_fd >= 0) {
            if (send_protocol_message(server->clients[i].socket_fd, header, payload, payload_size)) {
                server->stats.messages_sent++;
                server->stats.bytes_transferred += sizeof(ProtocolHeader) + payload_size;
            }
        }
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
}

void protocol_server_send_to_client(ProtocolServer* server, int client_id, 
                                const ProtocolHeader* header, const void* payload, 
                                ByteCount payload_size) {
    if (!server || !server->running || client_id < 0 || client_id >= MAX_CLIENTS) return;
    
    pthread_mutex_lock(&server->clients_mutex);
    
    if (server->clients[client_id].socket_fd >= 0) {
        if (send_protocol_message(server->clients[client_id].socket_fd, header, payload, payload_size)) {
            server->stats.messages_sent++;
            server->stats.bytes_transferred += sizeof(ProtocolHeader) + payload_size;
        }
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
}

ServerStats protocol_server_get_stats(const ProtocolServer* server) {
    if (!server) {
        ServerStats empty_stats = {0};
        return empty_stats;
    }
    
    ServerStats stats = server->stats;
    stats.active_connections = server->client_count;
    
    return stats;
}
