/**
 * @file types.h
 * @brief Centralized type definitions and constants for Smart Monitor system
 * 
 * This file contains semantic type aliases (strong typing) and constants
 * to improve code readability, maintainability and FFI safety.
 * Following Domain-Driven Design principles with strong typing.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// SYSTEM CONSTANTS
// =============================================================================

/** @brief System version string */
#define SYSTEM_VERSION "2.0.0"

/** @brief Maximum buffer sizes */
#define MAX_BUFFER_SIZE 4096
#define MAX_COMMAND_LENGTH 64
#define MAX_RESPONSE_LENGTH 128
#define MAX_DEVICE_PATH_LENGTH 256

/** @brief Default values */
#define DEFAULT_BAUDRATE 115200
#define DEFAULT_FRAME_WIDTH 640
#define DEFAULT_FRAME_HEIGHT 480
#define DEFAULT_MOTION_THRESHOLD 25
#define DEFAULT_AUDIO_SAMPLE_RATE 44100
#define DEFAULT_VIDEO_DEVICE "/dev/video0"
#define DEFAULT_HTTP_PORT 8080

/** @brief Protocol identifiers */
#define PROTOCOL_VERSION_MAJOR 1
#define PROTOCOL_VERSION_MINOR 0
#define PROTOCOL_MAGIC_BYTES 0x534D  // "SM" truncated to 16-bit

/** @brief Error codes */
#define ERROR_NONE 0
#define ERROR_INVALID_PARAMETER -1
#define ERROR_BUFFER_OVERFLOW -2
#define ERROR_DEVICE_NOT_FOUND -3
#define ERROR_COMMUNICATION_FAILED -4
#define ERROR_INITIALIZATION_FAILED -5

/** @brief Audio detection thresholds */
#define AUDIO_BABY_CRYING_THRESHOLD 0.7f
#define AUDIO_SCREAMING_THRESHOLD 0.9f
#define AUDIO_VOICE_ACTIVITY_THRESHOLD 0.3f

// =============================================================================
// SEMANTIC TYPE ALIASES (Strong Typing)
// =============================================================================

// --- Size and Count Types ---
typedef uint32_t ByteCount;        /**< Number of bytes in buffer/data */
typedef uint32_t PixelCount;       /**< Number of pixels in image */
typedef uint32_t FrameCount;       /**< Number of frames processed */
typedef uint32_t ObjectCount;      /**< Number of objects detected */
typedef uint32_t SampleCount;      /**< Number of audio samples */

// --- Dimension Types ---
typedef uint32_t PixelWidth;       /**< Width in pixels */
typedef uint32_t PixelHeight;      /**< Height in pixels */
typedef uint32_t FrameWidth;       /**< Frame width in pixels */
typedef uint32_t FrameHeight;      /**< Frame height in pixels */

// --- Timing Types ---
typedef uint64_t TimestampMs;      /**< Timestamp in milliseconds */
typedef uint64_t TimestampUs;      /**< Timestamp in microseconds */
typedef uint32_t DurationMs;       /**< Duration in milliseconds */
typedef uint32_t IntervalMs;       /**< Interval between operations */
typedef uint32_t TimeoutMs;        /**< Timeout duration in milliseconds */

// --- Data Types ---
typedef uint8_t* FrameBuffer;      /**< Pointer to frame data buffer */
typedef uint8_t* AudioBuffer;      /**< Pointer to audio data buffer */
typedef uint8_t* CommandBuffer;    /**< Pointer to command data buffer */
typedef uint8_t* ResponseBuffer;   /**< Pointer to response data buffer */

// --- Configuration Types ---
typedef uint32_t BaudRate;         /**< Serial communication baud rate */
typedef uint32_t SampleRate;       /**< Audio sampling rate in Hz */
typedef uint8_t MotionThreshold;   /**< Motion detection threshold (0-255) */
typedef float MotionLevel;         /**< Normalized motion level (0.0-1.0) */
typedef float ConfidenceLevel;     /**< Detection confidence (0.0-1.0) */

// --- Communication Types ---
typedef int32_t FileDescriptor;    /**< File descriptor for I/O operations */
typedef uint8_t DeviceId;          /**< Unique device identifier */
typedef uint16_t PortNumber;       /**< Network port number */

// --- Status and State Types ---
typedef uint8_t SystemState;       /**< Current system state */
typedef uint8_t DeviceState;       /**< Device operational state */
typedef uint8_t ConnectionState;   /**< Network connection state */
typedef uint8_t ErrorCode;         /**< Error code from operations */

// --- Sensor Data Types ---
typedef float TemperatureC;       /**< Temperature in Celsius */
typedef float HumidityPercent;     /**< Humidity percentage */
typedef uint16_t LightLux;         /**< Light level in Lux */
typedef uint8_t BatteryPercent;    /**< Battery percentage */
typedef uint8_t CpuUsagePercent;   /**< CPU usage percentage */
typedef uint8_t MemoryUsagePercent; /**< Memory usage percentage */

// --- Accelerometer Types ---
typedef int16_t AccelerometerAxis; /**< Accelerometer axis reading */

// --- Processing Latency Types ---
typedef float LatencyMs;           /**< Processing latency in milliseconds */

// =============================================================================
// SYSTEM STATES ENUMERATIONS
// =============================================================================

/** @brief System operational states */
typedef enum {
    SYSTEM_STATE_INITIALIZING = 0,   /**< System is starting up */
    SYSTEM_STATE_READY = 1,          /**< System ready for operation */
    SYSTEM_STATE_RUNNING = 2,        /**< System actively processing */
    SYSTEM_STATE_PAUSED = 3,         /**< System temporarily paused */
    SYSTEM_STATE_ERROR = 4,          /**< System in error state */
    SYSTEM_STATE_SHUTTING_DOWN = 5,  /**< System shutting down */
    SYSTEM_STATE_OFFLINE = 6          /**< System offline */
} SystemStateEnum;

/** @brief Device operational states */
typedef enum {
    DEVICE_STATE_DISCONNECTED = 0,   /**< Device not connected */
    DEVICE_STATE_CONNECTING = 1,     /**< Device establishing connection */
    DEVICE_STATE_CONNECTED = 2,      /**< Device connected and ready */
    DEVICE_STATE_ACTIVE = 3,         /**< Device actively operating */
    DEVICE_STATE_ERROR = 4,          /**< Device in error state */
    DEVICE_STATE_DISABLED = 5         /**< Device disabled */
} DeviceStateEnum;

/** @brief Connection states */
typedef enum {
    CONNECTION_STATE_DISCONNECTED = 0, /**< No connection */
    CONNECTION_STATE_CONNECTING = 1,   /**< Establishing connection */
    CONNECTION_STATE_CONNECTED = 2,    /**< Connection established */
    CONNECTION_STATE_AUTHENTICATED = 3, /**< Connection authenticated */
    CONNECTION_STATE_ERROR = 4,         /**< Connection error */
    CONNECTION_STATE_TIMEOUT = 5       /**< Connection timeout */
} ConnectionStateEnum;

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

/** @brief Protocol magic numbers and identifiers */
#define PROTOCOL_HEADER_SIZE 16
#define PROTOCOL_MAX_PACKET_SIZE 1024

// =============================================================================
// HARDWARE CONSTANTS
// =============================================================================

/** @brief UART configuration constants */
#define UART_BUFFER_SIZE 1024
#define UART_DEFAULT_DEVICE "/dev/ttyS0"
#define UART_TIMEOUT_MS 1000

/** @brief I2C configuration constants */
#define I2C_DEFAULT_BUS 1
#define I2C_MAX_DEVICES 127
#define I2C_TIMEOUT_MS 100

/** @brief SPI configuration constants */
#define SPI_MAX_SPEED 1000000  /**< 1MHz maximum SPI speed */
#define SPI_DEFAULT_MODE 0
#define SPI_TIMEOUT_MS 500

// =============================================================================
// AUDIO/VIDEO CONSTANTS
// =============================================================================

/** @brief Video format constants */
#define VIDEO_FORMAT_YUV420 0
#define VIDEO_FORMAT_RGB888 1
#define VIDEO_FORMAT_GRAYSCALE 2
#define VIDEO_FORMAT_MJPEG 3

/** @brief Audio format constants */
#define AUDIO_FORMAT_PCM16 0
#define AUDIO_FORMAT_PCM8 1
#define AUDIO_FORMAT_ULAW 2
#define AUDIO_FORMAT_ALAW 3

/** @brief Frame rate constants */
#define DEFAULT_FPS 30
#define MIN_FPS 1
#define MAX_FPS 120

// =============================================================================
// UTILITY MACROS
// =============================================================================

/** @brief Convert milliseconds to seconds */
#define MS_TO_SECONDS(ms) ((ms) / 1000)

/** @brief Convert seconds to milliseconds */
#define SECONDS_TO_MS(sec) ((sec) * 1000)

/** @brief Calculate buffer size for frame */
#define FRAME_BUFFER_SIZE(width, height) ((width) * (height) * 3)

/** @brief Validate timestamp */
#define IS_VALID_TIMESTAMP(ts) ((ts) > 0)

/** @brief Validate dimensions */
#define IS_VALID_DIMENSION(w, h) (((w) > 0) && ((h) > 0))

/** @brief Get array size */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifdef __cplusplus
}
#endif

#endif // TYPES_H
