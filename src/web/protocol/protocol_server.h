#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "../../core/common/smart_monitor_protocol.h"
#include "../../core/agent/data_agent.h"
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct protocol_server protocol_server_t;

// Server configuration
typedef struct {
    uint16_t port;
    int max_clients;
    bool enable_websocket;
    uint32_t heartbeat_interval_ms;
    uint32_t timeout_ms;
} server_config_t;

// Client connection structure
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    uint32_t last_activity;
    uint32_t sequence_in;
    uint32_t sequence_out;
    bool authenticated;
    uint8_t buffer[4096];
    size_t buffer_len;
} client_connection_t;

// Server lifecycle functions
protocol_server_t* protocol_server_create(const server_config_t* config);
void protocol_server_destroy(protocol_server_t* server);

// Server control functions
bool protocol_server_start(protocol_server_t* server);
void protocol_server_stop(protocol_server_t* server);
bool protocol_server_is_running(const protocol_server_t* server);

// Agent integration
void protocol_server_set_agent(protocol_server_t* server, data_agent_t* agent);

// Client management functions
int protocol_server_get_client_count(const protocol_server_t* server);
void protocol_server_broadcast_message(protocol_server_t* server, const protocol_header_t* header, 
                                  const void* payload, size_t payload_size);
void protocol_server_send_to_client(protocol_server_t* server, int client_id, 
                                const protocol_header_t* header, const void* payload, 
                                size_t payload_size);

// Statistics
typedef struct {
    uint32_t total_connections;
    uint32_t active_connections;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_transferred;
    uint64_t start_time;
} server_stats_t;

server_stats_t protocol_server_get_stats(const protocol_server_t* server);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SERVER_H
