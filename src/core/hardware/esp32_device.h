#ifndef ESP32_DEVICE_H
#define ESP32_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool initialized;
    char* uart_device;
    int baudrate;
    bool connected;
    uint32_t last_heartbeat;
} esp32_device_t;

typedef struct {
    float temperature;
    float humidity;
    uint16_t light_level;
    uint8_t battery_level;
    bool motion_detected;
    uint32_t timestamp;
} esp32_sensor_data_t;

typedef struct {
    uint8_t device_id;
    char device_type[32];
    char firmware_version[16];
    uint32_t uptime;
    uint8_t signal_strength;
} esp32_device_info_t;

// ESP32 device functions
esp32_device_t* esp32_create(const char* uart_device);
bool esp32_initialize(esp32_device_t* esp32, int baudrate);
void esp32_destroy(esp32_device_t* esp32);
bool esp32_connect(esp32_device_t* esp32);
bool esp32_send_command(esp32_device_t* esp32, const char* command);
esp32_sensor_data_t esp32_read_sensors(esp32_device_t* esp32);
esp32_device_info_t esp32_get_device_info(esp32_device_t* esp32);
bool esp32_send_ble_beacon(esp32_device_t* esp32, const char* data);

// Mock ESP32 functions
esp32_device_t* esp32_create_mock(void);
void esp32_simulate_sensor_data(esp32_device_t* esp32);

#endif // ESP32_DEVICE_H
