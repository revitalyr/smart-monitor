# Testing Guide for Smart Monitor

This guide explains how to test the Smart Monitor system without a physical camera.

## 🎥 Virtual Camera Setup

### Method 1: V4L2 Loopback (Recommended)

This creates a virtual `/dev/video0` device that behaves exactly like a real camera.

```bash
# Setup virtual camera (requires sudo)
sudo ./scripts/setup_virtual_camera.sh

# Feed test video with moving ball (best for motion detection)
gst-launch-1.0 videotestsrc pattern=ball ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    v4l2sink device=/dev/video0
```

### Available Test Patterns

| Pattern | Description | Use Case |
|---------|-------------|----------|
| `ball` | Moving bouncing ball | Motion detection testing |
| `smpte` | Color bars | Static scene testing |
| `snow` | Random noise | Noise tolerance testing |
| `black` | Black screen | No motion baseline |
| `white` | White screen | Brightness testing |

## 🧪 Motion Detection Tests

### Automated Test Suite

```bash
# Run comprehensive motion detection test
./scripts/test_motion_detection.sh
```

This test runs three scenarios:
1. **Static scene** (10 seconds) - should detect no motion
2. **Moving ball** (15 seconds) - should detect continuous motion
3. **Alternating patterns** (30 seconds) - should detect motion bursts

### Manual Testing

```bash
# Terminal 1: Start Smart Monitor
./build/firmware/cpp_core/smart_monitor

# Terminal 2: Feed test patterns
# Static test
gst-launch-1.0 videotestsrc pattern=black ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    v4l2sink device=/dev/video0

# Motion test
gst-launch-1.0 videotestsrc pattern=ball ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    v4l2sink device=/dev/video0
```

Monitor results at:
- Web Dashboard: http://localhost:8080
- Metrics API: http://localhost:8080/metrics
- Health API: http://localhost:8080/health

## 📹 Test Video Files

### Generate Test Videos

```bash
# Create various test videos
./scripts/generate_test_video.sh
```

This creates videos in `test_videos/`:
- `moving_ball.mp4` - Continuous motion
- `color_bars.mp4` - Static reference
- `snow_noise.mp4` - Random noise
- `alternating_motion.mp4` - Motion bursts

### Loop Test Video

```bash
# Loop video for continuous testing
gst-launch-1.0 filesrc location=test_videos/moving_ball.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! \
    videoconvert ! v4l2sink device=/dev/video0
```

## 🌐 RTSP/WebRTC Testing

### Setup RTSP Server

```bash
# Start MediaMTX RTSP server
./scripts/setup_rtsp_server.sh

# This creates:
# - RTSP endpoint: rtsp://localhost:8554/test
# - Web interface: http://localhost:8889
```

### Test WebRTC Streaming

1. Start Smart Monitor with virtual camera
2. Open web dashboard at http://localhost:8080
3. Click "Start Stream" to test WebRTC
4. Should see live video from test pattern

## 🔍 Validation Checklist

### Motion Detection Validation

- [ ] Static scene shows 0 motion events
- [ ] Moving ball triggers continuous motion events
- [ ] Motion level values are reasonable (0.0-1.0)
- [ ] Motion count increases only when expected

### Video Pipeline Validation

- [ ] Camera status shows "Active"
- [ ] Frame counter increases steadily
- [ ] Video stream is smooth (30 FPS)
- [ ] No memory leaks during long runs

### Web Dashboard Validation

- [ ] Dashboard loads without errors
- [ ] Metrics update in real-time
- [ ] Charts display data correctly
- [ ] WebRTC streaming works
- [ ] Responsive design on mobile

### API Validation

- [ ] `/health` returns `{"status":"ok"}`
- [ ] `/metrics` returns valid JSON with all fields
- [ ] `/webrtc/offer` returns SDP offer
- [ ] CORS headers are present

## 🛠️ Troubleshooting

### Common Issues

**Virtual camera not found:**
```bash
# Check if module is loaded
lsmod | grep v4l2loopback

# Check device permissions
ls -la /dev/video0
```

**GStreamer pipeline errors:**
```bash
# Check available plugins
gst-inspect-1.0 | grep v4l2
gst-inspect-1.0 | grep x264
```

**Motion detection not working:**
```bash
# Check Rust module
cd firmware/rust_modules/motion
cargo test

# Check FFI integration
./build/firmware/cpp_core/tests/test_rust_bridge
```

### Performance Tuning

**Reduce CPU usage:**
```bash
# Lower frame rate
gst-launch-1.0 videotestsrc pattern=ball ! \
    video/x-raw,width=640,height=480,framerate=15/1 ! \
    v4l2sink device=/dev/video0
```

**Reduce motion detection sensitivity:**
```bash
# Edit config file
sed -i 's/threshold=20/threshold=30/' /etc/smart-monitor.conf
```

## 🧹 Cleanup

```bash
# Clean up all test environment
./scripts/cleanup_test_env.sh
```

This stops:
- Virtual camera device
- RTSP server
- GStreamer processes
- Smart Monitor
- Test videos (optional)

## 📊 Performance Benchmarks

Expected performance on typical hardware:

| Metric | Expected Value |
|--------|----------------|
| CPU Usage | 10-20% (single core) |
| Memory Usage | 50-100 MB |
| Motion Detection Latency | < 50ms |
| WebRTC Latency | < 200ms |
| Frame Processing Rate | 30 FPS |

Use these benchmarks to validate your setup is performing correctly.
