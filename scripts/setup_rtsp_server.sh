#!/bin/bash

echo "[*] RTSP Server Setup for WebRTC Testing"
echo "======================================"

# Check if Docker is available
if ! command -v docker >/dev/null 2>&1; then
    echo "[✗] Docker is required but not installed"
    exit 1
fi

echo "[*] Starting MediaMTX RTSP server..."
docker run -d --name mediamtx-rtsp \
    -p 8554:8554 \
    -p 8888:8888 \
    -p 8889:8889 \
    -p 8890:8890/udp \
    bluenviron/mediamtx

if [ $? -eq 0 ]; then
    echo "[✓] MediaMTX server started successfully"
    echo "[*] RTSP endpoint: rtsp://localhost:8554/test"
    echo "[*] Web interface: http://localhost:8889"
    echo ""
    echo "[*] Feeding test stream..."
    
    # Wait for server to start
    sleep 3
    
    # Feed test video stream
    gst-launch-1.0 videotestsrc pattern=ball ! \
        video/x-raw,width=640,height=480,framerate=30/1 ! \
        x264enc tune=zerolatency bitrate=2000 ! \
        rtspclientsink location=rtsp://localhost:8554/test 2>/dev/null &
    
    STREAM_PID=$!
    
    echo "[✓] Test stream started"
    echo "[*] Stream URL: rtsp://localhost:8554/test"
    echo "[*] Stop with: kill $STREAM_PID"
    echo "[*] Stop server with: docker stop mediamtx-rtsp"
    
    echo ""
    echo "[*] Testing with Smart Monitor:"
    echo "   1. Modify config to use RTSP source"
    echo "   2. Or use the stream in WebRTC tests"
    
else
    echo "[✗] Failed to start MediaMTX server"
    exit 1
fi
