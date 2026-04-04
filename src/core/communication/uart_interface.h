#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool initialized;
    int fd;
    char* device_path;
    int baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    char parity;
} uart_interface_t;

typedef struct {
    char command[64];
    char response[128];
    uint32_t timestamp;
} uart_command_t;

// UART interface functions
uart_interface_t* uart_create(const char* device_path);
bool uart_initialize(uart_interface_t* uart, int baudrate);
void uart_destroy(uart_interface_t* uart);
bool uart_write(uart_interface_t* uart, const char* data, int len);
int uart_read(uart_interface_t* uart, char* buffer, int max_len);
bool uart_send_command(uart_interface_t* uart, const char* cmd, char* response, int max_len);

// Mock UART functions
uart_interface_t* uart_create_mock(void);
void uart_process_commands(uart_interface_t* uart);
void uart_log_message(uart_interface_t* uart, const char* level, const char* message);

#endif // UART_INTERFACE_H
