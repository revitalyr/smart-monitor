/**
 * @file uart_interface.h
 * @brief UART communication interface for Smart Monitor system
 */
#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct UartInterface
 * @brief UART interface configuration and state
 */
typedef struct {
    bool initialized;
    FileDescriptor fd;
    char* device_path;
    BaudRate baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    char parity;
} UartInterface;

/**
 * @struct UartCommand
 * @brief UART command structure for communication
 */
typedef struct {
    char command[MAX_COMMAND_LENGTH];
    char response[MAX_RESPONSE_LENGTH];
    TimestampMs timestamp;
} UartCommand;

// UART interface functions
UartInterface* uart_create(const char* device_path);
bool uart_initialize(UartInterface* uart, BaudRate baudrate);
void uart_destroy(UartInterface* uart);
bool uart_write(UartInterface* uart, const char* data, ByteCount len);
ByteCount uart_read(UartInterface* uart, char* buffer, ByteCount max_len);
bool uart_send_command(UartInterface* uart, const char* cmd, char* response, ByteCount max_len);

// Mock UART functions
UartInterface* uart_create_mock(void);
void uart_process_commands(UartInterface* uart);
void uart_log_message(UartInterface* uart, const char* level, const char* message);

#ifdef __cplusplus
}
#endif

#endif // UART_INTERFACE_H
