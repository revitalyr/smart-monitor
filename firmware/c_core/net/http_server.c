#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

struct http_server {
    int port;
    int server_fd;
    bool running;
    pthread_t server_thread;
    
    metrics_data_t metrics;
    
    metrics_callback_t metrics_callback;
    health_callback_t health_callback;
    webrtc_callback_t webrtc_callback;
};

static void* server_loop(void* arg);
static void handle_request(int client_fd, http_server_t* server);
static void send_response(int fd, const char* status, const char* content_type, const char* body);
static void send_cors_headers(int fd);

http_server_t* http_server_create(int port) {
    http_server_t* server = malloc(sizeof(http_server_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(http_server_t));
    server->port = port;
    server->server_fd = -1;
    
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
    if (!server || server->running) {
        return false;
    }
    
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("socket");
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server->server_fd);
        server->server_fd = -1;
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server->port);
    
    if (bind(server->server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server->server_fd);
        server->server_fd = -1;
        return false;
    }
    
    if (listen(server->server_fd, 10) < 0) {
        perror("listen");
        close(server->server_fd);
        server->server_fd = -1;
        return false;
    }
    
    server->running = true;
    
    if (pthread_create(&server->server_thread, NULL, server_loop, server) != 0) {
        perror("pthread_create");
        server->running = false;
        close(server->server_fd);
        server->server_fd = -1;
        return false;
    }
    
    printf("HTTP Server started on port %d\n", server->port);
    return true;
}

void http_server_stop(http_server_t* server) {
    if (!server || !server->running) {
        return;
    }
    
    server->running = false;
    
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    pthread_join(server->server_thread, NULL);
}

bool http_server_is_running(const http_server_t* server) {
    return server && server->running;
}

void http_server_set_metrics_callback(http_server_t* server, metrics_callback_t callback) {
    if (server) {
        server->metrics_callback = callback;
    }
}

void http_server_set_health_callback(http_server_t* server, health_callback_t callback) {
    if (server) {
        server->health_callback = callback;
    }
}

void http_server_set_webrtc_callback(http_server_t* server, webrtc_callback_t callback) {
    if (server) {
        server->webrtc_callback = callback;
    }
}

metrics_data_t* http_server_get_metrics(http_server_t* server) {
    return server ? &server->metrics : NULL;
}

static void* server_loop(void* arg) {
    http_server_t* server = (http_server_t*)arg;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->running) {
                perror("accept");
            }
            continue;
        }
        
        handle_request(client_fd, server);
        close(client_fd);
    }
    
    return NULL;
}

static void handle_request(int client_fd, http_server_t* server) {
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read <= 0) {
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    send_cors_headers(client_fd);
    
    if (strstr(buffer, "GET /health")) {
        char* response = server->health_callback ? server->health_callback() : generate_health_json();
        send_response(client_fd, "200 OK", "application/json", response);
        if (response) free(response);
    }
    else if (strstr(buffer, "GET /metrics")) {
        char* response = server->metrics_callback ? server->metrics_callback() : generate_metrics_json(&server->metrics);
        send_response(client_fd, "200 OK", "application/json", response);
        if (response) free(response);
    }
    else if (strstr(buffer, "GET /webrtc/offer")) {
        char* response = server->webrtc_callback ? server->webrtc_callback() : generate_webrtc_sdp();
        send_response(client_fd, "200 OK", "application/sdp", response);
        if (response) free(response);
    }
    else if (strstr(buffer, "OPTIONS")) {
        send_response(client_fd, "200 OK", "text/plain", "");
    }
    else {
        // Simple HTML response for root
        const char* html = 
            "<!DOCTYPE html><html><head><title>Smart Monitor</title></head>"
            "<body><h1>Smart Monitor</h1><p>Video monitoring system</p></body></html>";
        send_response(client_fd, "200 OK", "text/html", html);
    }
}

static void send_cors_headers(int fd) {
    const char* cors_headers = 
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
    send(fd, cors_headers, strlen(cors_headers), 0);
}

static void send_response(int fd, const char* status, const char* content_type, const char* body) {
    char response[8192];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, content_type, strlen(body), body);
    
    send(fd, response, strlen(response), 0);
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
        metrics->motion_events,
        metrics->motion_level,
        metrics->frames_processed,
        metrics->camera_active ? "true" : "false",
        metrics->last_motion_time,
        metrics->uptime
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
