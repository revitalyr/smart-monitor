//! Smart Monitor Types Library
//! 
//! This crate provides centralized type definitions and constants
//! for the Smart Monitor system, following Domain-Driven Design
//! principles with strong typing.

pub mod types;

// Re-export all types for convenience
pub use types::*;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_type_creation() {
        // Test that our types can be created
        let timestamp: TimestampMs = 12345;
        let width: FrameWidth = 640;
        let height: FrameHeight = 480;
        let threshold: MotionThreshold = 25;
        
        assert_eq!(timestamp, 12345);
        assert_eq!(width, 640);
        assert_eq!(height, 480);
        assert_eq!(threshold, 25);
    }

    #[test]
    fn test_utility_functions() {
        let width: FrameWidth = 640;
        let height: FrameHeight = 480;
        let buffer_size = frame_buffer_size(width, height);
        
        assert_eq!(buffer_size, 640 * 480 * 3);
        
        assert!(is_valid_timestamp(1000));
        assert!(!is_valid_timestamp(0));
        
        assert!(is_valid_dimension(640, 480));
        assert!(!is_valid_dimension(0, 480));
    }

    #[test]
    fn test_enums() {
        let state = SystemStateEnum::Running;
        assert_eq!(state as u8, 2);
        
        let device_state = DeviceStateEnum::Connected;
        assert_eq!(device_state as u8, 2);
        
        let conn_state = ConnectionStateEnum::Connected;
        assert_eq!(conn_state as u8, 2);
    }
}
