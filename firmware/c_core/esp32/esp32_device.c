#include "esp32_device.h"
#include "../comm/uart_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ESP32_BAUDRATE 115200
#define HEARTBEAT_TIMEOUT 5000 // 5 seconds

esp32_device_t* esp32_create(const char* uart_device) {
    esp32_device_t* esp32 = malloc(sizeof(esp32_device_t));
    if (!esp32) {
        return NULL;
    }
    
    memset(esp32, 0, sizeof(esp32_device_t));
    esp32->uart_device = strdup(uart_device ? uart_device : "/dev/ttyUSB0");
    esp32->baudrate = ESP32_BAUDRATE;
    esp32->connected = false;
    
    return esp32;
}

esp32_device_t* esp32_create_mock(void) {
    esp32_device_t* esp32 = esp32_create("/mock/esp32");
    if (esp32) {
        esp32->initialized = true;
        esp32->connected = true;
        esp32->last_heartbeat = time(NULL);
    }
    return esp32;
}

bool esp32_initialize(esp32_device_t* esp32, int baudrate) {
    if (!esp32) {
        return false;
    }
    
    esp32->baudrate = baudrate;
    esp32->initialized = true;
    
    return true;
}

void esp32_destroy(esp32_device_t* esp32) {
    if (!esp32) {
        return;
    }
    
    if (esp32->uart_device) {
        free(esp32->uart_device);
    }
    
    free(esp32);
}

bool esp32_connect(esp32_device_t* esp32) {
    if (!esp32 || !esp32->initialized) {
        return false;
    }
    
    // Try to connect via UART
    uart_interface_t* uart = uart_create(esp32->uart_device);
    if (!uart) {
        return false;
    }
    
    if (!uart_initialize(uart, esp32->baudrate)) {
        uart_destroy(uart);
        return false;
    }
    
    // Send handshake
    char response[128];
    if (uart_send_command(uart, "ESP32:HANDSHAKE", response, sizeof(response))) {
        if (strstr(response, "OK")) {
            esp32->connected = true;
            esp32->last_heartbeat = time(NULL);
            uart_destroy(uart);
            return true;
        }
    }
    
    uart_destroy(uart);
    return false;
}

bool esp32_send_command(esp32_device_t* esp32, const char* command) {
    if (!esp32 || !esp32->connected || !command) {
        return false;
    }
    
    // Mock implementation
    esp32->last_heartbeat = time(NULL);
    return true;
}

esp32_sensor_data_t esp32_read_sensors(esp32_device_t* esp32) {
    esp32_sensor_data_t data = {0};
    
    if (!esp32 || !esp32->connected) {
        return data;
    }
    
    // Mock sensor data generation
    static uint32_t counter = 0;
    counter++;
    
    data.timestamp = time(NULL);
    
    // Temperature with daily variation
    data.temperature = 22.0f + sin(counter * 0.01) * 3.0f;
    
    // Humidity with some variation
    data.humidity = 45.0f + cos(counter * 0.008) * 15.0f;
    
    // Light level (simulated day/night cycle)
    data.light_level = (sin(counter * 0.005) + 1.0f) * 512.0f;
    
    // Battery level decreasing over time
    data.battery_level = 100 - (counter % 1000) / 10;
    
    // Random motion detection
    data.motion_detected = (counter % 100) > 95;
    
    return data;
}

esp32_device_info_t esp32_get_device_info(esp32_device_t* esp32) {
    esp32_device_info_t info = {0};
    
    if (!esp32 || !esp32->connected) {
        return info;
    }
    
    // Mock device info
    info.device_id = 1;
    strcpy(info.device_type, "ESP32-DevKitC");
    strcpy(info.firmware_version, "1.0.0");
    info.uptime = time(NULL) - esp32->last_heartbeat;
    info.signal_strength = 75 + (rand() % 25);
    
    return info;
}

bool esp32_send_ble_beacon(esp32_device_t* esp32, const char* data) {
    if (!esp32 || !esp32->connected || !data) {
        return false;
    }
    
    // Mock BLE beacon transmission
    char command[256];
    snprintf(command, sizeof(command), "BLE:BEACON:%s", data);
    
    return esp32_send_command(esp32, command);
}

void esp32_simulate_sensor_data(esp32_device_t* esp32) {
    // This function would be called periodically to update sensor data
    // The actual simulation happens in esp32_read_sensors()
}
