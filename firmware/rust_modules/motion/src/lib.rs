use std::slice;

/// Motion detection result
#[repr(C)]
pub struct MotionResult {
    pub motion_detected: bool,
    pub motion_level: f32,
    pub changed_pixels: u32,
}

/// Detect motion between two frames
#[no_mangle]
pub extern "C" fn detect_motion(
    prev: *const u8,
    curr: *const u8,
    len: usize,
    threshold: u8,
) -> bool {
    if prev.is_null() || curr.is_null() || len == 0 {
        return false;
    }
    
    let prev = unsafe { slice::from_raw_parts(prev, len) };
    let curr = unsafe { slice::from_raw_parts(curr, len) };

    let mut diff_count = 0;
    let threshold_i16 = threshold as i16;

    for i in 0..len {
        let diff = (prev[i] as i16 - curr[i] as i16).abs();
        if diff > threshold_i16 {
            diff_count += 1;
        }
    }

    // 5% threshold for motion detection
    diff_count > len / 20
}

/// Advanced motion detection with detailed metrics
#[no_mangle]
pub extern "C" fn detect_motion_advanced(
    prev: *const u8,
    curr: *const u8,
    width: u32,
    height: u32,
    threshold: u8,
) -> MotionResult {
    if prev.is_null() || curr.is_null() || width == 0 || height == 0 {
        return MotionResult {
            motion_detected: false,
            motion_level: 0.0,
            changed_pixels: 0,
        };
    }
    
    let len = (width * height) as usize;
    let prev = unsafe { slice::from_raw_parts(prev, len) };
    let curr = unsafe { slice::from_raw_parts(curr, len) };

    let mut changed_pixels = 0u32;
    let mut total_diff = 0u64;
    let threshold_i16 = threshold as i16;

    for i in 0..len {
        let diff = (prev[i] as i16 - curr[i] as i16).abs();
        if diff > threshold_i16 {
            changed_pixels += 1;
            total_diff += diff as u64;
        }
    }

    let motion_ratio = changed_pixels as f32 / len as f32;
    let motion_level = if changed_pixels > 0 {
        (total_diff as f32 / changed_pixels as f32) / 255.0
    } else {
        0.0
    };

    MotionResult {
        motion_detected: motion_ratio > 0.05, // 5% threshold
        motion_level,
        changed_pixels,
    }
}

/// Initialize motion detection engine
#[no_mangle]
pub extern "C" fn motion_init() -> bool {
    true // Always successful for now
}

/// Cleanup motion detection engine
#[no_mangle]
pub extern "C" fn motion_cleanup() {
    // Nothing to clean up for now
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_motion_detection() {
        let a = vec![0u8; 100];
        let b = vec![100u8; 100];
        
        assert!(detect_motion(a.as_ptr(), b.as_ptr(), 100, 10));
    }

    #[test]
    fn test_no_motion() {
        let a = vec![50u8; 100];
        let b = vec![52u8; 100];
        
        assert!(!detect_motion(a.as_ptr(), b.as_ptr(), 100, 5));
    }

    #[test]
    fn test_advanced_motion() {
        let width = 10;
        let height = 10;
        let mut a = vec![0u8; width * height];
        let mut b = vec![0u8; width * height];
        
        // Add some motion
        for i in 0..20 {
            b[i] = 100;
        }
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), width, height, 10);
        
        assert!(result.motion_detected);
        assert!(result.changed_pixels == 20);
        assert!(result.motion_level > 0.0);
    }
}
