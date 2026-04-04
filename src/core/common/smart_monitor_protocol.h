#ifndef SMART_MONITOR_PROTOCOL_H
#define SMART_MONITOR_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Protocol magic bytes for identification
#define PROTOCOL_MAGIC 0x5A4D  // "MZ" in hex
#define PROTOCOL_VERSION 1

// Message types
typedef enum {
    MSG_TYPE_HEARTBEAT = 0x01,
    MSG_TYPE_SENSOR_DATA = 0x02,
    MSG_TYPE_AUDIO_DATA = 0x03,
    MSG_TYPE_VIDEO_FRAME = 0x04,
    MSG_TYPE_MOTION_ALERT = 0x05,
    MSG_TYPE_SYSTEM_STATUS = 0x06,
    MSG_TYPE_COMMAND = 0x07,
    MSG_TYPE_RESPONSE = 0x08
} message_type_t;

// Command types
typedef enum {
    CMD_START_CAPTURE = 0x01,
    CMD_STOP_CAPTURE = 0x02,
    CMD_GET_STATUS = 0x03,
    CMD_SET_THRESHOLD = 0x04,
    CMD_RESET = 0x05
} command_type_t;

// Protocol header structure
typedef struct __attribute__((packed)) {
    uint16_t magic;           // Protocol magic bytes
    uint8_t version;          // Protocol version
    uint8_t type;            // Message type
    uint32_t sequence;       // Sequence number
    uint32_t timestamp;       // Unix timestamp
    uint16_t payload_length;  // Length of payload
    uint16_t checksum;       // CRC16 checksum
} protocol_header_t;

// Sensor data payload
typedef struct __attribute__((packed)) {
    float temperature;        // Temperature in Celsius
    float humidity;          // Humidity percentage
    uint16_t light_level;   // Light level (0-1023)
    bool motion_detected;    // Motion detection status
    uint32_t reserved;      // Reserved for future use
} sensor_data_t;

// Audio data payload
typedef struct __attribute__((packed)) {
    float noise_level;        // Current noise level (0.0-1.0)
    bool voice_activity;      // Voice activity detected
    bool baby_crying;       // Baby crying detected
    bool screaming;          // Screaming detected
    uint16_t peak_frequency; // Peak frequency in Hz
    uint16_t reserved;      // Reserved for future use
} audio_data_t;

// Motion alert payload
typedef struct __attribute__((packed)) {
    float motion_level;      // Motion intensity (0.0-1.0)
    uint16_t x_coord;       // Motion X coordinate
    uint16_t y_coord;       // Motion Y coordinate
    uint32_t frame_number;   // Frame number
    uint8_t confidence;      // Detection confidence (0-100)
    uint8_t reserved;       // Reserved for future use
} motion_alert_t;

// System status payload
typedef struct __attribute__((packed)) {
    uint8_t cpu_usage;      // CPU usage percentage
    uint8_t memory_usage;   // Memory usage percentage
    uint16_t fps;          // Current FPS
    uint32_t frames_processed; // Total frames processed
    uint32_t uptime;        // System uptime in seconds
    bool camera_active;      // Camera status
    bool recording_active;   // Recording status
    uint16_t reserved;      // Reserved for future use
} system_status_t;

// Command payload
typedef struct __attribute__((packed)) {
    uint8_t command;        // Command type
    uint8_t target;         // Target component
    uint16_t parameter;      // Command parameter
    uint32_t value;         // Command value
    uint32_t reserved;       // Reserved for future use
} command_payload_t;

// Response payload
typedef struct __attribute__((packed)) {
    uint8_t status;         // Response status (0=success, 1=error)
    uint8_t error_code;     // Error code if status=1
    uint16_t data_length;    // Response data length
    uint32_t reserved;       // Reserved for future use
    // Followed by optional data field
} response_payload_t;

// Protocol functions
uint16_t protocol_calculate_crc(const uint8_t* data, size_t length);
bool protocol_validate_header(const protocol_header_t* header);
void protocol_create_header(protocol_header_t* header, message_type_t type, 
                        uint32_t sequence, uint32_t timestamp, 
                        uint16_t payload_length);

#ifdef __cplusplus
}
#endif

#endif // SMART_MONITOR_PROTOCOL_H
