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

/// Advanced motion detection with adaptive threshold
#[no_mangle]
pub extern "C" fn detect_motion_advanced(
    prev: *const u8,
    curr: *const u8,
    width: u32,
    height: u32,
    threshold: u8,
) -> MotionResult {
    // Safety checks
    if prev.is_null() || curr.is_null() {
        return MotionResult {
            motion_detected: false,
            motion_level: 0.0,
            changed_pixels: 0,
        };
    }
    
    if width == 0 || height == 0 || width > 10000 || height > 10000 {
        return MotionResult {
            motion_detected: false,
            motion_level: 0.0,
            changed_pixels: 0,
        };
    }
    
    let total_pixels = width as usize * height as usize;
    
    // Check for reasonable frame size
    if total_pixels > 10_000_000 { // 10MP limit
        return MotionResult {
            motion_detected: false,
            motion_level: 0.0,
            changed_pixels: 0,
        };
    }
    
    let prev_slice = unsafe { std::slice::from_raw_parts(prev, total_pixels) };
    let curr_slice = unsafe { std::slice::from_raw_parts(curr, total_pixels) };
    
    let mut changed_pixels = 0u32;
    let mut total_diff = 0u32;
    let mut noise_level = 0u32;
    
    for (i, (&a, &b)) in prev_slice.iter().zip(curr_slice.iter()).enumerate() {
        let diff = if a > b { a - b } else { b - a };
        if diff > threshold as u8 {
            changed_pixels += 1;
            total_diff += diff as u32;
        }
        noise_level += diff as u32;
    }
    
    let motion_ratio = changed_pixels as f32 / total_pixels as f32;
    let avg_noise = noise_level as f32 / total_pixels as f32;
    let motion_level = if changed_pixels > 0 {
        total_diff as f32 / (changed_pixels as f32 * 255.0)
    } else {
        0.0
    };
    
    // Adaptive threshold based on noise level and motion characteristics
    let base_threshold = 0.005; // 0.5% base threshold
    let noise_factor = (avg_noise / 10.0).min(0.02); // Noise compensation
    let adaptive_threshold = base_threshold + noise_factor;
    
    // Additional detection criteria for better sensitivity
    let significant_motion = changed_pixels > 100 && motion_level > 0.1;
    let high_density_motion = motion_ratio > 0.001 && total_diff > 500;
    
    let motion_detected = motion_ratio > adaptive_threshold || 
                         significant_motion || 
                         high_density_motion;
    
    MotionResult {
        motion_detected,
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
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), width as u32, height as u32, 10);
        
        assert!(result.motion_detected);
        assert!(result.changed_pixels == 20);
        assert!(result.motion_level > 0.0);
    }

    #[test]
    fn test_null_pointers() {
        let result = detect_motion_advanced(std::ptr::null(), std::ptr::null(), 10, 10, 10);
        assert!(!result.motion_detected);
        assert_eq!(result.changed_pixels, 0);
        assert_eq!(result.motion_level, 0.0);
    }

    #[test]
    fn test_zero_dimensions() {
        let a = vec![0u8; 100];
        let b = vec![100u8; 100];
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), 0, 10, 10);
        assert!(!result.motion_detected);
        assert_eq!(result.changed_pixels, 0);
        assert_eq!(result.motion_level, 0.0);
    }

    #[test]
    fn test_edge_cases() {
        // Test with single pixel
        let a = vec![0u8; 1];
        let b = vec![255u8; 1];
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), 1, 1, 1);
        assert!(result.motion_detected);
        assert_eq!(result.changed_pixels, 1);
        assert!(result.motion_level > 0.9);
    }

    #[test]
    fn test_motion_threshold_boundary() {
        let width = 20;
        let height = 10;
        let total_pixels = width * height;
        let a = vec![0u8; total_pixels];
        let mut b = vec![0u8; total_pixels];
        
        // More than 5% of pixels changed to ensure detection
        let changed_pixels = total_pixels / 20 + 1; // 11 pixels = 5.5%
        for i in 0..changed_pixels {
            b[i] = 100;  // High difference to ensure detection
        }
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), width as u32, height as u32, 5); // Lower threshold
        println!("Total pixels: {}, Changed pixels: {}, Motion detected: {}", 
                 total_pixels, changed_pixels, result.motion_detected);
        assert!(result.motion_detected);
        assert_eq!(result.changed_pixels, changed_pixels as u32);
    }

    #[test]
    fn test_motion_level_calculation() {
        let width = 10;
        let height = 10;
        let a = vec![0u8; width * height];
        let mut b = vec![0u8; width * height];
        
        // Set exactly half pixels to max difference
        for i in 0..50 {
            b[i] = 255;
        }
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), width as u32, height as u32, 10);
        assert!(result.motion_detected);
        assert_eq!(result.changed_pixels, 50);
        assert!((result.motion_level - 1.0).abs() < 0.01);
    }

    #[test]
    fn test_identical_frames() {
        let a = vec![128u8; 100];
        let b = vec![128u8; 100];
        
        assert!(!detect_motion(a.as_ptr(), b.as_ptr(), 100, 10));
        
        let result = detect_motion_advanced(a.as_ptr(), b.as_ptr(), 10 as u32, 10 as u32, 10);
        assert!(!result.motion_detected);
        assert_eq!(result.changed_pixels, 0);
        assert_eq!(result.motion_level, 0.0);
    }

    #[test]
    fn test_motion_init_cleanup() {
        assert!(motion_init());
        motion_cleanup(); // Should not panic
    }

    #[test]
    fn test_different_thresholds() {
        let a = vec![0u8; 100];
        let b = vec![50u8; 100];
        
        // Low threshold should detect motion
        assert!(detect_motion(a.as_ptr(), b.as_ptr(), 100, 10));
        
        // High threshold should not detect motion
        assert!(!detect_motion(a.as_ptr(), b.as_ptr(), 100, 60));
    }
}
