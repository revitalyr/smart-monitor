#ifndef SMART_MONITOR_PROTOCOL_H
#define SMART_MONITOR_PROTOCOL_H

#include "types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
} MessageType;

// Command types
typedef enum {
    CMD_START_CAPTURE = 0x01,
    CMD_STOP_CAPTURE = 0x02,
    CMD_GET_STATUS = 0x03,
    CMD_SET_THRESHOLD = 0x04,
    CMD_RESET = 0x05
} CommandType;

// Protocol header structure
typedef struct __attribute__((packed)) {
    uint16_t magic;           // Protocol magic bytes
    uint8_t version;          // Protocol version
    uint8_t type;            // Message type
    uint32_t sequence;       // Sequence number
    TimestampMs timestamp;    // Unix timestamp
    uint16_t payload_length;  // Length of payload
    uint16_t checksum;       // CRC16 checksum
} ProtocolHeader;

// Sensor data payload
typedef struct __attribute__((packed)) {
    float temperature_c;        // Temperature in Celsius
    float humidity_percent;      // Humidity percentage
    uint16_t light_level_lux;   // Light level in Lux
    bool pir_motion_detected;    // Motion detection status
    uint32_t reserved;      // Reserved for future use
} SensorData;

// Audio data payload
typedef struct __attribute__((packed)) {
    MotionLevel noise_level;     // Current noise level (0.0-1.0)
    bool voice_activity;      // Voice activity detected
    bool baby_crying;       // Baby crying detected
    bool screaming;          // Screaming detected
    uint16_t peak_frequency_hz; // Peak frequency in Hz
    uint16_t reserved;      // Reserved for future use
} AudioData;

// Motion alert payload
typedef struct __attribute__((packed)) {
    MotionLevel motion_level;    // Motion intensity (0.0-1.0)
    uint16_t x_coord;       // Motion X coordinate
    uint16_t y_coord;       // Motion Y coordinate
    FrameCount frame_number;   // Frame number
    ConfidenceLevel confidence; // Detection confidence (0-100)
    uint8_t reserved;       // Reserved for future use
} MotionAlert;

// System status payload
typedef struct __attribute__((packed)) {
    uint8_t cpu_usage_percent;      // CPU usage percentage
    uint8_t memory_usage_percent;   // Memory usage percentage
    uint16_t fps;          // Current FPS
    FrameCount frames_processed; // Total frames processed
    DurationMs uptime_ms;        // System uptime in milliseconds
    bool camera_active;      // Camera status
    bool recording_active;   // Recording status
    uint16_t reserved;      // Reserved for future use
} SystemStatus;

// Command payload
typedef struct __attribute__((packed)) {
    uint8_t command;        // Command type
    uint8_t target;         // Target component
    uint16_t parameter;      // Command parameter
    uint32_t value;         // Command value
    uint32_t reserved;       // Reserved for future use
} CommandPayload;

// Response payload
typedef struct __attribute__((packed)) {
    uint8_t status;         // Response status (0=success, 1=error)
    ErrorCode error_code;     // Error code if status=1
    uint16_t data_length;    // Response data length
    uint32_t reserved;       // Reserved for future use
    // Followed by optional data field
} ResponsePayload;

// Protocol functions
uint16_t protocol_calculate_crc(const uint8_t* data, ByteCount length);
bool protocol_validate_header(const ProtocolHeader* header);
void protocol_create_header(ProtocolHeader* header, MessageType type, 
                        uint32_t sequence, TimestampMs timestamp, 
                        uint16_t payload_length);

#ifdef __cplusplus
}
#endif

#endif // SMART_MONITOR_PROTOCOL_H
