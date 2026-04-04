#ifndef SMART_MONITOR_CONSTANTS_H
#define SMART_MONITOR_CONSTANTS_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// SEMANTIC TYPE ALIASES
// =============================================================================

// Hardware-related types
typedef float temperature_c_t;        // Temperature in Celsius
typedef float humidity_percent_t;      // Humidity percentage (0-100)
typedef int light_lux_t;             // Light level in lux
typedef int battery_percent_t;         // Battery percentage (0-100)
typedef int signal_dbm_t;             // Signal strength in dBm

// Audio/Visual analysis types
typedef float noise_level_t;          // Noise level (0.0-1.0)
typedef float motion_level_t;          // Motion level (0.0-1.0)
typedef float frequency_hz_t;         // Frequency in Hertz
typedef int sleep_score_t;            // Sleep score (0-100)
typedef int frame_count_t;           // Frame counter
typedef int motion_events_count_t;     // Motion events counter

// System metrics types
typedef int uptime_seconds_t;         // Uptime in seconds
typedef float cpu_usage_percent_t;    // CPU usage percentage
typedef float memory_usage_percent_t;  // Memory usage percentage
typedef uint64_t timestamp_t;        // Unix timestamp
typedef uint32_t sample_count_t;      // Sample count
typedef uint32_t sleep_duration_t;    // Sleep duration in minutes
typedef int sample_rate_t;            // Audio sample rate
typedef int window_size_t;            // FFT window size
typedef float* fft_buffer_t;          // FFT buffer pointer
typedef float* frequency_bins_t;       // Frequency bins pointer
typedef int port_t;                    // Network port number
typedef int socket_fd_t;                // Socket file descriptor
typedef int file_fd_t;                  // File descriptor
typedef uint16_t accelerometer_data_t;  // Accelerometer data
typedef uint16_t gyroscope_data_t;      // Gyroscope data
typedef uint16_t magnetometer_data_t;    // Magnetometer data

// =============================================================================
// HARDWARE CONSTANTS
// =============================================================================

// I2C sensor constants
#define I2C_SENSOR_TEMP_MIN         -40.0f
#define I2C_SENSOR_TEMP_MAX          85.0f
#define I2C_SENSOR_HUMIDITY_MIN     0.0f
#define I2C_SENSOR_HUMIDITY_MAX     100.0f
#define I2C_SENSOR_LIGHT_MIN         0
#define I2C_SENSOR_LIGHT_MAX         65535

// ESP32 constants
#define ESP32_BATTERY_MIN            0
#define ESP32_BATTERY_MAX            100
#define ESP32_SIGNAL_MIN             -120
#define ESP32_SIGNAL_MAX             0
#define ESP32_TEMP_MIN              -40.0f
#define ESP32_TEMP_MAX              85.0f
#define ESP32_HUMIDITY_MIN          0.0f
#define ESP32_HUMIDITY_MAX          100.0f

// IMU sensor constants
#define IMU_ACCEL_MIN               -32768
#define IMU_ACCEL_MAX               32767
#define IMU_GYRO_MIN                -32768
#define IMU_GYRO_MAX                32767

// System constants
#define SYSTEM_CPU_MIN               0.0f
#define SYSTEM_CPU_MAX               100.0f
#define SYSTEM_MEMORY_MIN            0.0f
#define SYSTEM_MEMORY_MAX            100.0f
#define SYSTEM_TEMP_MIN              0.0f
#define SYSTEM_TEMP_MAX              125.0f

// =============================================================================
// AUDIO/VISUAL ANALYSIS CONSTANTS
// =============================================================================

// Audio analysis constants
#define AUDIO_NOISE_LEVEL_MIN        0.0f
#define AUDIO_NOISE_LEVEL_MAX        1.0f
#define AUDIO_VOICE_ACTIVITY_THRESHOLD  0.3f
#define AUDIO_BABY_CRYING_THRESHOLD  0.7f
#define AUDIO_SCREAMING_THRESHOLD    0.9f

// Motion detection constants
#define MOTION_LEVEL_MIN             0.0f
#define MOTION_LEVEL_MAX             1.0f
#define MOTION_DETECTION_THRESHOLD    0.5f

// Sleep analysis constants
#define SLEEP_SCORE_MIN              0
#define SLEEP_SCORE_MAX              100
#define SLEEP_MOTION_WEIGHT          0.6f
#define SLEEP_NOISE_WEIGHT           0.4f
#define SLEEP_DEEP_THRESHOLD         0.1f

// =============================================================================
// NETWORK AND PROTOCOL CONSTANTS
// =============================================================================

// HTTP server constants
#define HTTP_SERVER_PORT             8080
#define HTTP_SERVER_PORT_STR         "8080"
#define HTTP_RESPONSE_BUFFER_SIZE    8192
#define HTTP_REQUEST_BUFFER_SIZE     4096

// API endpoint paths
#define API_AUDIO_STATUS             "/audio/status"
#define API_AUDIO_TOGGLE             "/audio/toggle"
#define API_SENSORS_I2C             "/sensors/i2c"
#define API_SENSORS_IMU             "/sensors/imu"
#define API_SENSORS_TOGGLE          "/sensors/toggle"
#define API_ESP32_STATUS            "/esp32/status"
#define API_ESP32_TOGGLE            "/esp32/toggle"
#define API_SLEEP_SCORE             "/sleep/score"
#define API_SYSTEM_STATUS           "/system/status"
#define API_SYSTEM_LOGS             "/system/logs"
#define API_SYSTEM_CALIBRATE        "/system/calibrate"
#define API_ANALYZE_VIDEO           "/analyze/video"
#define API_ANALYZE_STATUS          "/analyze/status"

// =============================================================================
// LOG AND MESSAGE CONSTANTS
// =============================================================================

// Log levels
#define LOG_LEVEL_ERROR             "ERROR"
#define LOG_LEVEL_WARN              "WARN"
#define LOG_LEVEL_INFO              "INFO"
#define LOG_LEVEL_DEBUG             "DEBUG"
#define LOG_LEVEL_SENSOR            "SENSOR"
#define LOG_LEVEL_ALERT             "ALERT"
#define LOG_LEVEL_DATA              "DATA"

// System messages
#define MSG_SYSTEM_STARTING         "Smart Monitor starting..."
#define MSG_CAMERA_INIT             "Camera initialized successfully"
#define MSG_MOTION_DETECTOR_INIT    "Motion detector initialized successfully"
#define MSG_HTTP_SERVER_STARTED     "HTTP server started on port %d"
#define MSG_AUDIO_CAPTURE_INIT      "Audio capture initialized"
#define MSG_I2C_SENSOR_INIT         "I2C sensor initialized"
#define MSG_ALL_COMPONENTS_INIT     "All components initialized"
#define MSG_BABY_CRYING_DETECTED   "Baby crying detected!"
#define MSG_SYSTEM_SHUTTING_DOWN    "Shutting down Smart Monitor..."

// Component status messages
#define STATUS_ENABLED               "enabled"
#define STATUS_DISABLED              "disabled"
#define STATUS_ACTIVE               "active"
#define STATUS_INACTIVE             "inactive"
#define STATUS_CONNECTED            "connected"
#define STATUS_DISCONNECTED         "disconnected"
#define STATUS_CALIBRATING          "calibrating"
#define STATUS_READY                "ready"

// Quality descriptors
#define QUALITY_EXCELLENT           "excellent"
#define QUALITY_GOOD                "good"
#define QUALITY_FAIR                "fair"
#define QUALITY_POOR                "poor"

// =============================================================================
// UI AND DISPLAY CONSTANTS
// =============================================================================

// Chart data points limit
#define CHART_MAX_DATA_POINTS       20

// Update intervals (in milliseconds)
#define UPDATE_INTERVAL_DASHBOARD    1000
#define UPDATE_INTERVAL_SENSORS      500
#define UPDATE_INTERVAL_AUDIO        200
#define UPDATE_INTERVAL_VIDEO        33  // ~30 FPS

// Animation durations
#define ANIMATION_MOTION_TEST_MS    3000
#define ANIMATION_CALIBRATION_MS     2000

// =============================================================================
// FILE PATH CONSTANTS
// =============================================================================

#define HTML_FILE_ENHANCED          "web_interface_enhanced.html"
#define HTML_FILE_FALLBACK          "web_interface.html"

// =============================================================================
// MATH CONSTANTS
// =============================================================================//

#define PI                          3.14159265359f
#define DEG_TO_RAD                  (PI / 180.0f)
#define RAD_TO_DEG                  (180.0f / PI)

#endif // SMART_MONITOR_CONSTANTS_H
