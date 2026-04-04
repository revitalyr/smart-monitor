#include "uart_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

#define DEFAULT_BAUDRATE 115200
#define UART_BUFFER_SIZE 1024

uart_interface_t* uart_create(const char* device_path) {
    uart_interface_t* uart = malloc(sizeof(uart_interface_t));
    if (!uart) {
        return NULL;
    }
    
    memset(uart, 0, sizeof(uart_interface_t));
    uart->device_path = strdup(device_path ? device_path : "/dev/ttyS0");
    uart->baudrate = DEFAULT_BAUDRATE;
    uart->data_bits = 8;
    uart->stop_bits = 1;
    uart->parity = 'N';
    uart->fd = -1;
    
    return uart;
}

uart_interface_t* uart_create_mock(void) {
    uart_interface_t* uart = uart_create("/mock/uart");
    if (uart) {
        uart->initialized = true;
    }
    return uart;
}

bool uart_initialize(uart_interface_t* uart, int baudrate) {
    if (!uart) {
        return false;
    }
    
    uart->baudrate = baudrate;
    
    // Try to open real UART device
    uart->fd = open(uart->device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart->fd < 0) {
        // Use mock mode
        uart->initialized = true;
        return true;
    }
    
    // Configure serial port
    struct termios options;
    tcgetattr(uart->fd, &options);
    
    // Set baudrate
    speed_t speed = B115200;
    switch (baudrate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default: speed = B115200; break;
    }
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    
    // Configure data format
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE; // Clear data bits
    options.c_cflag |= CS8; // 8 data bits
    
    // Set raw mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;
    
    // Set timeout
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10; // 1 second timeout
    
    tcsetattr(uart->fd, TCSANOW, &options);
    
    uart->initialized = true;
    return true;
}

void uart_destroy(uart_interface_t* uart) {
    if (!uart) {
        return;
    }
    
    if (uart->fd >= 0) {
        close(uart->fd);
    }
    
    if (uart->device_path) {
        free(uart->device_path);
    }
    
    free(uart);
}

bool uart_write(uart_interface_t* uart, const char* data, int len) {
    if (!uart || !uart->initialized || !data) {
        return false;
    }
    
    if (uart->fd < 0) {
        // Mock mode - just print to stdout for demo
        printf("UART TX: %.*s\n", len, data);
        return true;
    }
    
    int written = write(uart->fd, data, len);
    return written == len;
}

int uart_read(uart_interface_t* uart, char* buffer, int max_len) {
    if (!uart || !uart->initialized || !buffer) {
        return -1;
    }
    
    if (uart->fd < 0) {
        // Mock mode - simulate incoming data
        static uint32_t counter = 0;
        counter++;
        
        if (counter % 100 == 0) {
            snprintf(buffer, max_len, "STATUS:OK,MOTION:%d,TEMP:%d\n", 
                    counter % 10, 20 + (counter % 10));
            return strlen(buffer);
        }
        return 0;
    }
    
    return read(uart->fd, buffer, max_len);
}

bool uart_send_command(uart_interface_t* uart, const char* cmd, char* response, int max_len) {
    if (!uart || !cmd || !response) {
        return false;
    }
    
    // Send command
    char cmd_with_crlf[256];
    snprintf(cmd_with_crlf, sizeof(cmd_with_crlf), "%s\r\n", cmd);
    
    if (!uart_write(uart, cmd_with_crlf, strlen(cmd_with_crlf))) {
        return false;
    }
    
    // Wait for response
    usleep(100000); // 100ms delay
    
    // Read response
    int len = uart_read(uart, response, max_len - 1);
    if (len > 0) {
        response[len] = '\0';
        
        // Remove trailing CRLF
        while (len > 0 && (response[len-1] == '\r' || response[len-1] == '\n')) {
            response[--len] = '\0';
        }
        return true;
    }
    
    response[0] = '\0';
    return false;
}

void uart_process_commands(uart_interface_t* uart) {
    char buffer[UART_BUFFER_SIZE];
    
    while (uart_read(uart, buffer, sizeof(buffer) - 1) > 0) {
        buffer[strlen(buffer)] = '\0';
        
        // Process commands
        if (strstr(buffer, "STATUS")) {
            uart_write(uart, "OK:RUNNING,MOTION:0,TEMP:22\n", 27);
        } else if (strstr(buffer, "RESET")) {
            uart_write(uart, "OK:RESET\n", 10);
        } else if (strstr(buffer, "PING")) {
            uart_write(uart, "PONG\n", 5);
        } else {
            uart_write(uart, "ERROR:UNKNOWN\n", 14);
        }
    }
}

void uart_log_message(uart_interface_t* uart, const char* level, const char* message) {
    char log_msg[256];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(log_msg, sizeof(log_msg), "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\r\n",
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            level, message);
    
    uart_write(uart, log_msg, strlen(log_msg));
}
