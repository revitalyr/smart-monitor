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
        perror("setsockopt SO_REUSEADDR");
        close(server->server_fd);
        server->server_fd = -1;
        return false;
    }
    
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEPORT");
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
    else if (strstr(buffer, "POST /analyze/video")) {
        // Video analysis endpoint
        const char* response = "{\"status\":\"started\",\"message\":\"Video analysis started in mock mode\",\"motion_detected\":true}";
        send_response(client_fd, "200 OK", "application/json", response);
    }
    else if (strstr(buffer, "GET /analyze/status")) {
        // Return analysis status
        char* json_response = malloc(256);
        snprintf(json_response, 256,
            "{\"status\":\"active\",\"motion_events\":%d,\"frames_processed\":%d,\"motion_level\":%.2f}",
            server->metrics.motion_events, server->metrics.frames_processed, server->metrics.motion_level);
        send_response(client_fd, "200 OK", "application/json", json_response);
        free(json_response);
    }
    else if (strstr(buffer, "OPTIONS")) {
        send_response(client_fd, "200 OK", "text/plain", "");
    }
    else {
        // Serve HTML interface from file - try multiple paths
        FILE* html_file = NULL;
        const char* paths[] = {
            "web_interface.html",
            "./web_interface.html", 
            "../web_interface.html",
            "../../firmware/c_core/web_interface.html",
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
                send_response(client_fd, "200 OK", "text/html", html_content);
                free(html_content);
            }
            fclose(html_file);
        } else {
            // Fallback HTML with inline interface
            const char* html = 
                "<!DOCTYPE html><html><head><title>Smart Monitor</title>"
                "<style>body{font-family:Arial;margin:40px;background:#f5f5f5}.container{max-width:800px;margin:0 auto;background:white;padding:30px;border-radius:10px}h1{color:#333;text-align:center}.btn{background:#4CAF50;color:white;padding:12px 24px;border:none;border-radius:5px;cursor:pointer;margin:10px 5px}.btn:hover{background:#45a049}.status{margin:20px 0;padding:15px;border-radius:5px;font-weight:bold}.status.success{background:#d4edda;color:#155724}.status.error{background:#f8d7da;color:#721c24}.metrics{margin:20px 0;padding:15px;background:#f8f9fa;border-radius:5px}.video-container{margin:20px 0;text-align:center}canvas{border:2px solid #333;background:#000}</style>"
                "</head><body>"
                "<div class='container'>"
                "<h1>Smart Monitor</h1>"
                "<p>Video monitoring system with motion detection</p>"
                "<div class='video-container'>"
                "<h3>Test Video Feed (Mock Mode)</h3>"
                "<canvas id='videoCanvas' width='640' height='480'></canvas>"
                "<p style='color:#666;font-size:14px'>Moving white object simulates motion detection</p>"
                "</div>"
                "<button class='btn' onclick='startAnalysis()'>Start Analysis</button>"
                "<button class='btn' onclick='stopAnalysis()'>Stop Analysis</button>"
                "<button class='btn' onclick='checkStatus()'>Check Status</button>"
                "<div id='status' class='status' style='display:none'></div>"
                "<div id='metrics' class='metrics' style='display:none'>"
                "<h3>Real-time Metrics</h3><div id='metricsContent'></div></div>"
                "</div>"
                "<script>"
                "let analysisActive=false,statusInterval=null,videoAnimationId=null,frameCounter=0;"
                "function showStatus(m,t){const s=document.getElementById('status');s.textContent=m;s.className='status '+t;s.style.display='block';}"
                "function animateVideo(){const c=document.getElementById('videoCanvas').getContext('2d');c.fillStyle='#808080';c.fillRect(0,0,640,480);const x=(frameCounter*10)%(640-100)+50,y=(frameCounter*5)%(480-100)+50;c.fillStyle='#FFFFFF';c.fillRect(x,y,80,80);c.fillStyle='#FF0000';c.fillRect(10,10,10,10);frameCounter++;videoAnimationId=requestAnimationFrame(animateVideo);}"
                "function startVideoAnimation(){if(!videoAnimationId)animateVideo();}"
                "function stopVideoAnimation(){if(videoAnimationId){cancelAnimationFrame(videoAnimationId);videoAnimationId=null;}}"
                "function startAnalysis(){fetch('/analyze/video',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:'test'})}).then(r=>r.json()).then(d=>{showStatus(d.message||'Analysis started','success');analysisActive=true;startStatusUpdates();startVideoAnimation();}).catch(e=>showStatus('Failed: '+e.message,'error'));}"
                "function stopAnalysis(){analysisActive=false;if(statusInterval){clearInterval(statusInterval);statusInterval=null;}stopVideoAnimation();showStatus('Analysis stopped','success');}"
                "function checkStatus(){fetch('/analyze/status').then(r=>r.json()).then(d=>{const m=document.getElementById('metrics'),mc=document.getElementById('metricsContent');mc.innerHTML='<p><strong>Status:</strong> '+d.status+'</p><p><strong>Motion Events:</strong> '+d.motion_events+'</p><p><strong>Frames:</strong> '+d.frames_processed+'</p><p><strong>Motion Level:</strong> '+(d.motion_level?d.motion_level.toFixed(2):'0.00')+'</p>';m.style.display='block';}).catch(e=>showStatus('Failed: '+e.message,'error'));}"
                "function startStatusUpdates(){if(!statusInterval)statusInterval=setInterval(checkStatus,2000);}"
                "window.addEventListener('load',startVideoAnimation);"
                "</script></body></html>";
            send_response(client_fd, "200 OK", "text/html", html);
        }
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
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
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
