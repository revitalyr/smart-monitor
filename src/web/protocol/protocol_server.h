#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "../../core/common/types.h"
#include "../../core/common/smart_monitor_protocol.h"
#include "../../core/agent/data_agent.h"
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct protocol_server ProtocolServer;

// Server configuration
typedef struct {
    PortNumber port;
    int max_clients;
    bool enable_websocket;
    IntervalMs heartbeat_interval_ms;
    TimeoutMs timeout_ms;
} ServerConfig;

// Client connection structure
typedef struct {
    FileDescriptor socket_fd;
    struct sockaddr_in address;
    TimestampMs last_activity;
    uint32_t sequence_in;
    uint32_t sequence_out;
    bool authenticated;
    uint8_t buffer[MAX_BUFFER_SIZE];
    ByteCount buffer_len;
} ClientConnection;

// Server lifecycle functions
ProtocolServer* protocol_server_create(const ServerConfig* config);
void protocol_server_destroy(ProtocolServer* server);

// Server control functions
bool protocol_server_start(ProtocolServer* server);
void protocol_server_stop(ProtocolServer* server);
bool protocol_server_is_running(const ProtocolServer* server);

// Agent integration
void protocol_server_set_agent(ProtocolServer* server, DataAgent* agent);

// Client management functions
int protocol_server_get_client_count(const ProtocolServer* server);
void protocol_server_broadcast_message(ProtocolServer* server, const ProtocolHeader* header, 
                                  const void* payload, ByteCount payload_size);
void protocol_server_send_to_client(ProtocolServer* server, int client_id, 
                                const ProtocolHeader* header, const void* payload, 
                                ByteCount payload_size);

// Statistics
typedef struct {
    uint32_t total_connections;
    uint32_t active_connections;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t bytes_transferred;
    TimestampMs start_time;
} ServerStats;

ServerStats protocol_server_get_stats(const ProtocolServer* server);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SERVER_H
