/**
 * @file globals.h
 * @brief Global variables and declarations for Smart Monitor system
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"
#include "communication/uart_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global variables declaration
extern UartInterface* g_uart;

// System state variables
extern SystemState g_system_state;
extern TimestampMs g_system_start_time;
extern FrameCount g_total_frames_processed;

#ifdef __cplusplus
}
#endif

#endif // GLOBALS_H
