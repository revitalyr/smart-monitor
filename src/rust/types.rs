//! Centralized type definitions and constants for Smart Monitor system
//! 
//! This module contains semantic type aliases (strong typing) and constants
//! to improve code readability, maintainability and FFI safety.
//! Following Domain-Driven Design principles with strong typing.

pub mod types {
    use std::time::Duration;

    // =============================================================================
    // SYSTEM CONSTANTS
    // =============================================================================

    /// System version string
    pub const SYSTEM_VERSION: &str = "2.0.0";

    /// Maximum buffer sizes
    pub const MAX_BUFFER_SIZE: usize = 4096;
    pub const MAX_COMMAND_LENGTH: usize = 64;
    pub const MAX_RESPONSE_LENGTH: usize = 128;
    pub const MAX_DEVICE_PATH_LENGTH: usize = 256;

    /// Default values
    pub const DEFAULT_BAUDRATE: u32 = 115200;
    pub const DEFAULT_FRAME_WIDTH: u32 = 640;
    pub const DEFAULT_FRAME_HEIGHT: u32 = 480;
    pub const DEFAULT_MOTION_THRESHOLD: u8 = 25;
    pub const DEFAULT_AUDIO_SAMPLE_RATE: u32 = 44100;

    /// Protocol identifiers
    pub const PROTOCOL_VERSION_MAJOR: u8 = 1;
    pub const PROTOCOL_VERSION_MINOR: u8 = 0;
    pub const PROTOCOL_MAGIC_BYTES: u32 = 0x534D4D54; // "SMMT"

    /// Error codes
    pub const ERROR_NONE: i32 = 0;
    pub const ERROR_INVALID_PARAMETER: i32 = -1;
    pub const ERROR_BUFFER_OVERFLOW: i32 = -2;
    pub const ERROR_DEVICE_NOT_FOUND: i32 = -3;
    pub const ERROR_COMMUNICATION_FAILED: i32 = -4;
    pub const ERROR_INITIALIZATION_FAILED: i32 = -5;

    // =============================================================================
    // SEMANTIC TYPE ALIASES (Strong Typing)
    // =============================================================================

    // --- Size and Count Types ---
    pub type ByteCount = u32;        ///< Number of bytes in buffer/data
    pub type PixelCount = u32;       ///< Number of pixels in image
    pub type FrameCount = u32;       ///< Number of frames processed
    pub type ObjectCount = u32;      ///< Number of objects detected
    pub type SampleCount = u32;      ///< Number of audio samples

    // --- Dimension Types ---
    pub type PixelWidth = u32;       ///< Width in pixels
    pub type PixelHeight = u32;      ///< Height in pixels
    pub type FrameWidth = u32;       ///< Frame width in pixels
    pub type FrameHeight = u32;      ///< Frame height in pixels

    // --- Timing Types ---
    pub type TimestampMs = u64;      ///< Timestamp in milliseconds
    pub type TimestampUs = u64;      ///< Timestamp in microseconds
    pub type DurationMs = u32;       ///< Duration in milliseconds
    pub type IntervalMs = u32;       ///< Interval between operations
    pub type TimeoutMs = u32;        ///< Timeout duration in milliseconds

    // --- Configuration Types ---
    pub type BaudRate = u32;         ///< Serial communication baud rate
    pub type SampleRate = u32;       ///< Audio sampling rate in Hz
    pub type MotionThreshold = u8;   ///< Motion detection threshold (0-255)
    pub type MotionLevel = f32;      ///< Normalized motion level (0.0-1.0)
    pub type ConfidenceLevel = f32;  ///< Detection confidence (0.0-1.0)

    // --- Communication Types ---
    pub type FileDescriptor = i32;   ///< File descriptor for I/O operations
    pub type DeviceId = u8;          ///< Unique device identifier
    pub type PortNumber = u16;       ///< Network port number

    // --- Status and State Types ---
    pub type SystemState = u8;       ///< Current system state
    pub type DeviceState = u8;       ///< Device operational state
    pub type ConnectionState = u8;   ///< Network connection state
    pub type ErrorCode = i32;         ///< Error code from operations

    // =============================================================================
    // SYSTEM STATES ENUMERATIONS
    // =============================================================================

    /// System operational states
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    #[repr(u8)]
    pub enum SystemStateEnum {
        Initializing = 0,   ///< System is starting up
        Ready = 1,          ///< System ready for operation
        Running = 2,        ///< System actively processing
        Paused = 3,         ///< System temporarily paused
        Error = 4,          ///< System in error state
        ShuttingDown = 5,  ///< System shutting down
        Offline = 6,        ///< System offline
    }

    /// Device operational states
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    #[repr(u8)]
    pub enum DeviceStateEnum {
        Disconnected = 0,   ///< Device not connected
        Connecting = 1,     ///< Device establishing connection
        Connected = 2,      ///< Device connected and ready
        Active = 3,         ///< Device actively operating
        Error = 4,          ///< Device in error state
        Disabled = 5,       ///< Device disabled
    }

    /// Connection states
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    #[repr(u8)]
    pub enum ConnectionStateEnum {
        Disconnected = 0,   ///< No connection
        Connecting = 1,     ///< Establishing connection
        Connected = 2,      ///< Connection established
        Authenticated = 3,  ///< Connection authenticated
        Error = 4,          ///< Connection error
        Timeout = 5,        ///< Connection timeout
    }

    // =============================================================================
    // PROTOCOL CONSTANTS
    // =============================================================================

    /// Protocol magic numbers and identifiers
    pub const PROTOCOL_HEADER_SIZE: usize = 8;
    pub const PROTOCOL_CHECKSUM_SIZE: usize = 4;
    pub const PROTOCOL_MAX_PACKET_SIZE: usize = 1024;

    /// Command identifiers
    pub const CMD_PING: u8 = 0x01;
    pub const CMD_STATUS: u8 = 0x02;
    pub const CMD_CONFIGURE: u8 = 0x03;
    pub const CMD_START_CAPTURE: u8 = 0x04;
    pub const CMD_STOP_CAPTURE: u8 = 0x05;
    pub const CMD_GET_FRAME: u8 = 0x06;
    pub const CMD_MOTION_DETECT: u8 = 0x07;
    pub const CMD_RESET: u8 = 0xFF;

    /// Response identifiers
    pub const RESP_OK: u8 = 0x00;
    pub const RESP_ERROR: u8 = 0x01;
    pub const RESP_BUSY: u8 = 0x02;
    pub const RESP_INVALID_CMD: u8 = 0x03;
    pub const RESP_DATA_READY: u8 = 0x04;

    // =============================================================================
    // HARDWARE CONSTANTS
    // =============================================================================

    /// UART configuration constants
    pub const UART_BUFFER_SIZE: usize = 1024;
    pub const UART_DEFAULT_DEVICE: &str = "/dev/ttyS0";
    pub const UART_TIMEOUT_MS: u32 = 1000;

    /// I2C configuration constants
    pub const I2C_DEFAULT_BUS: u8 = 1;
    pub const I2C_MAX_DEVICES: u8 = 127;
    pub const I2C_TIMEOUT_MS: u32 = 100;

    /// SPI configuration constants
    pub const SPI_MAX_SPEED: u32 = 1_000_000;  // 1MHz maximum SPI speed
    pub const SPI_DEFAULT_MODE: u8 = 0;
    pub const SPI_TIMEOUT_MS: u32 = 500;

    // =============================================================================
    // AUDIO/VIDEO CONSTANTS
    // =============================================================================

    /// Video format constants
    pub const VIDEO_FORMAT_YUV420: u8 = 0;
    pub const VIDEO_FORMAT_RGB888: u8 = 1;
    pub const VIDEO_FORMAT_GRAYSCALE: u8 = 2;
    pub const VIDEO_FORMAT_MJPEG: u8 = 3;

    /// Audio format constants
    pub const AUDIO_FORMAT_PCM16: u8 = 0;
    pub const AUDIO_FORMAT_PCM8: u8 = 1;
    pub const AUDIO_FORMAT_ULAW: u8 = 2;
    pub const AUDIO_FORMAT_ALAW: u8 = 3;

    /// Frame rate constants
    pub const DEFAULT_FPS: u8 = 30;
    pub const MIN_FPS: u8 = 1;
    pub const MAX_FPS: u8 = 120;

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    /// Convert milliseconds to Duration
    pub fn ms_to_duration(ms: DurationMs) -> Duration {
        Duration::from_millis(ms as u64)
    }

    /// Convert Duration to milliseconds
    pub fn duration_to_ms(duration: Duration) -> DurationMs {
        duration.as_millis() as DurationMs
    }

    /// Calculate buffer size for frame
    pub fn frame_buffer_size(width: FrameWidth, height: FrameHeight) -> ByteCount {
        (width * height * 3) as ByteCount
    }

    /// Validate timestamp
    pub fn is_valid_timestamp(ts: TimestampMs) -> bool {
        ts > 0
    }

    /// Validate dimensions
    pub fn is_valid_dimension(width: FrameWidth, height: FrameHeight) -> bool {
        width > 0 && height > 0
    }

    /// Get array size (compile-time)
    #[macro_export]
    macro_rules! array_size {
        ($arr:expr) => {
            std::mem::size_of_val($arr) / std::mem::size_of::<std::slice::Iter<'_, _>>()
        };
    }
}

pub mod ffi {
    //! FFI-safe type definitions for C-Rust interoperability
    
    use super::types::*;
    use std::ffi::c_void;
    use std::ptr;

    // =============================================================================
    // FFI-SAFE STRUCTURES
    // =============================================================================

    /// Motion detection result (FFI-safe)
    #[repr(C)]
    #[derive(Debug, Clone, Copy)]
    pub struct MotionResult {
        pub motion_detected: bool,
        pub motion_level: MotionLevel,
        pub changed_pixels: PixelCount,
    }

    impl Default for MotionResult {
        fn default() -> Self {
            Self {
                motion_detected: false,
                motion_level: 0.0,
                changed_pixels: 0,
            }
        }
    }

    /// Motion detector handle (FFI-safe)
    #[repr(C)]
    #[derive(Debug)]
    pub struct MotionDetector {
        pub initialized: bool,
        // Internal state would be here
        _private: [u8; 0], // Prevent direct construction
    }

    // =============================================================================
    // FFI FUNCTION SIGNATURES
    // =============================================================================

    extern "C" {
        /// Initialize motion detection system
        pub fn motion_init() -> bool;

        /// Cleanup motion detection system
        pub fn motion_cleanup();

        /// Basic motion detection
        pub fn detect_motion(
            prev: *const u8,
            curr: *const u8,
            len: usize,
            threshold: u8,
        ) -> bool;

        /// Advanced motion detection
        pub fn detect_motion_advanced(
            prev: *const u8,
            curr: *const u8,
            width: u32,
            height: u32,
            threshold: u8,
        ) -> MotionResult;

        /// Create motion detector instance
        pub fn rust_detector_create() -> *mut MotionDetector;

        /// Initialize motion detector
        pub fn rust_detector_initialize(detector: *mut MotionDetector) -> bool;

        /// Destroy motion detector
        pub fn rust_detector_destroy(detector: *mut MotionDetector);

        /// Motion detection with detector
        pub fn rust_detector_detect_motion(
            detector: *mut MotionDetector,
            prev: *const u8,
            curr: *const u8,
            len: usize,
            threshold: u8,
        ) -> bool;

        /// Advanced motion detection with detector
        pub fn rust_detector_detect_motion_advanced(
            detector: *mut MotionDetector,
            prev: *const u8,
            curr: *const u8,
            width: u32,
            height: u32,
            threshold: u8,
        ) -> MotionResult;
    }
}

pub use types::*;
pub use ffi::*;
