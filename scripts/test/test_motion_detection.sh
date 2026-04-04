#!/bin/bash

echo "[*] Motion Detection Test Script"
echo "=============================="

# Check if virtual camera exists
if [ ! -e /dev/video0 ]; then
    echo "[✗] No camera device found at /dev/video0"
    echo "[*] Run sudo ./scripts/setup_virtual_camera.sh first"
    exit 1
fi

echo "[*] Starting motion detection test..."
echo "[*] This will test different scenarios:"
echo "   1. Static scene (no motion)"
echo "   2. Moving ball (continuous motion)"
echo "   3. Alternating patterns (motion bursts)"
echo ""

# Function to start smart monitor in background
start_monitor() {
    echo "[*] Starting Smart Monitor..."
    ./build/firmware/cpp_core/smart_monitor &
    MONITOR_PID=$!
    sleep 3  # Give it time to start
}

# Function to stop monitor
stop_monitor() {
    if [ ! -z "$MONITOR_PID" ]; then
        echo "[*] Stopping Smart Monitor..."
        kill $MONITOR_PID 2>/dev/null || true
        wait $MONITOR_PID 2>/dev/null || true
    fi
}

# Function to feed test pattern
feed_pattern() {
    local pattern=$1
    local duration=$2
    echo "[*] Feeding pattern: $pattern for ${duration}s"
    timeout $duration gst-launch-1.0 videotestsrc pattern=$pattern ! video/x-raw,width=640,height=480,framerate=30/1 ! v4l2sink device=/dev/video0 2>/dev/null &
    PATTERN_PID=$!
    sleep $duration
    kill $PATTERN_PID 2>/dev/null || true
}

# Test 1: Static scene
echo "[*] Test 1: Static scene (should detect no motion)"
start_monitor
feed_pattern "black" 10
echo "[*] Check metrics at http://localhost:8080/metrics"
sleep 2
stop_monitor
echo ""

# Test 2: Continuous motion
echo "[*] Test 2: Moving ball (should detect continuous motion)"
start_monitor
feed_pattern "ball" 15
echo "[*] Check metrics - motion events should be increasing"
sleep 2
stop_monitor
echo ""

# Test 3: Alternating patterns
echo "[*] Test 3: Alternating static/motion (should detect motion bursts)"
start_monitor
for i in {1..3}; do
    echo "[*] Cycle $i: Static for 5s, then motion for 5s"
    feed_pattern "black" 5
    feed_pattern "ball" 5
done
echo "[*] Check metrics - should see 3 motion bursts"
sleep 2
stop_monitor

echo ""
echo "[✓] Motion detection test completed!"
echo "[*] Review the logs and metrics to verify detection works correctly"
echo "[*] Web dashboard: http://localhost:8080"
