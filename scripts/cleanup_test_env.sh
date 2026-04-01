#!/bin/bash

echo "[*] Cleaning up test environment"
echo "==============================="

# Stop virtual camera
echo "[*] Stopping virtual camera..."
sudo modprobe -r v4l2loopback 2>/dev/null || echo "[*] v4l2loopback not loaded"

# Stop MediaMTX server
echo "[*] Stopping MediaMTX server..."
docker stop mediamtx-rtsp 2>/dev/null || echo "[*] MediaMTX not running"
docker rm mediamtx-rtsp 2>/dev/null || echo "[*] MediaMTX container not found"

# Kill any remaining GStreamer processes
echo "[*] Killing GStreamer processes..."
pkill -f "gst-launch-1.0" 2>/dev/null || echo "[*] No GStreamer processes found"

# Stop Smart Monitor if running
echo "[*] Stopping Smart Monitor..."
pkill -f "smart_monitor" 2>/dev/null || echo "[*] Smart Monitor not running"

# Clean up test videos (optional)
read -p "[*] Remove test videos? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf test_videos
    echo "[*] Test videos removed"
fi

echo "[✓] Test environment cleaned up!"
