#include "uart_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

// UART interface implementation
UartInterface* uart_create(const char* device_path) {
    UartInterface* uart = malloc(sizeof(UartInterface));
    if (!uart) {
        return NULL;
    }
    
    memset(uart, 0, sizeof(UartInterface));
    uart->device_path = strdup(device_path ? device_path : UART_DEFAULT_DEVICE);
    uart->baudrate = DEFAULT_BAUDRATE;
    uart->data_bits = 8;
    uart->stop_bits = 1;
    uart->parity = 'N';
    uart->fd = -1;
    
    return uart;
}

UartInterface* uart_create_mock(void) {
    UartInterface* uart = uart_create("/mock/uart");
    if (uart) {
        uart->initialized = true;
    }
    return uart;
}

bool uart_initialize(UartInterface* uart, BaudRate baudrate) {
    if (!uart) {
        return false;
    }
    
    uart->baudrate = baudrate;
    
    // Try to open real UART device
    uart->fd = open(uart->device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart->fd < 0) {
        // Use mock mode
        uart->initialized = true;
        printf("INFO: Using mock UART mode for device %s\n", uart->device_path);
        return true;
    }
    
    // Configure real UART
    struct termios options;
    tcgetattr(uart->fd, &options);
    
    // Set baud rate
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);
    
    // 8N1 configuration
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    // Enable receiver, ignore modem control lines
    options.c_cflag |= (CLOCAL | CREAD);
    
    // Raw input mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Raw output mode
    options.c_oflag &= ~OPOST;
    
    // Set timeout
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = UART_TIMEOUT_MS / 100;
    
    tcsetattr(uart->fd, TCSANOW, &options);
    uart->initialized = true;
    
    return true;
}

void uart_destroy(UartInterface* uart) {
    if (uart) {
        if (uart->fd >= 0) {
            close(uart->fd);
        }
        if (uart->device_path) {
            free(uart->device_path);
        }
        free(uart);
    }
}

bool uart_write(UartInterface* uart, const char* data, ByteCount len) {
    if (!uart || !uart->initialized || !data || len == 0) {
        return false;
    }
    
    if (uart->fd >= 0) {
        // Real UART write
        ByteCount written = write(uart->fd, data, len);
        return written == len;
    } else {
        // Mock mode - simulate write
        printf("UART WRITE: %.*s\n", (int)len, data);
        return true;
    }
}

ByteCount uart_read(UartInterface* uart, char* buffer, ByteCount max_len) {
    if (!uart || !uart->initialized || !buffer || max_len == 0) {
        return 0;
    }
    
    if (uart->fd >= 0) {
        // Real UART read
        return read(uart->fd, buffer, max_len);
    } else {
        // Mock mode - simulate read
        printf("UART READ: Waiting for data (mock)\n");
        return 0;
    }
}

bool uart_send_command(UartInterface* uart, const char* cmd, char* response, ByteCount max_len) {
    if (!uart || !cmd || !response) {
        return false;
    }
    
    // Send command
    ByteCount cmd_len = strlen(cmd);
    if (!uart_write(uart, cmd, cmd_len)) {
        return false;
    }
    
    // Add newline if not present
    if (cmd[cmd_len - 1] != '\n') {
        uart_write(uart, "\n", 1);
    }
    
    // Wait for response
    usleep(100000); // 100ms delay
    
    // Read response
    ByteCount read_len = uart_read(uart, response, max_len - 1);
    response[read_len] = '\0';
    
    return read_len > 0;
}

void uart_process_commands(UartInterface* uart) {
    if (!uart || !uart->initialized) {
        return;
    }
    
    char buffer[MAX_BUFFER_SIZE];
    ByteCount len = uart_read(uart, buffer, sizeof(buffer) - 1);
    
    if (len > 0) {
        buffer[len] = '\0';
        printf("UART received: %s\n", buffer);
        
        // Process commands here
        if (strstr(buffer, "STATUS")) {
            uart_write(uart, "OK: System ready\n", 18);
        } else if (strstr(buffer, "PING")) {
            uart_write(uart, "PONG\n", 5);
        }
    }
}

void uart_log_message(UartInterface* uart, const char* level, const char* message) {
    if (!uart || !level || !message) {
        return;
    }
    
    TimestampMs timestamp = (TimestampMs)time(NULL) * 1000;
    char log_entry[MAX_COMMAND_LENGTH];
    snprintf(log_entry, sizeof(log_entry), "[%lu] %s: %s\n", timestamp, level, message);
    
    uart_write(uart, log_entry, strlen(log_entry));
}
