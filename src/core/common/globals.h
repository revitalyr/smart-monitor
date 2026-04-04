#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include "communication/uart_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global variables declaration
extern uart_interface_t* g_uart;

#ifdef __cplusplus
}
#endif

#endif // GLOBALS_H
