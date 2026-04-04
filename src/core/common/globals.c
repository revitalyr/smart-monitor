/**
 * @file globals.c
 * @brief Global variables definitions for Smart Monitor system
 */
#include "globals.h"
#include <stddef.h>

// Global variables definition
UartInterface* g_uart = NULL;

// System state variables
SystemState g_system_state = SYSTEM_STATE_INITIALIZING;
TimestampMs g_system_start_time = 0;
FrameCount g_total_frames_processed = 0;
