#!/bin/bash

echo "[*] Setting up virtual V4L2 camera for testing"
echo "============================================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script requires sudo privileges for kernel module installation"
    exit 1
fi

# Install v4l2loopback module
echo "[*] Installing v4l2loopback-dkms..."
apt update
apt install -y v4l2loopback-dkms

# Remove existing module if loaded
echo "[*] Removing existing v4l2loopback module..."
modprobe -r v4l2loopback 2>/dev/null || true

# Load virtual camera device
echo "[*] Loading virtual camera device..."
modprobe v4l2loopback devices=1 video_nr=0 card_label="TestCam" exclusive_caps=1

# Verify device was created
if [ -e /dev/video0 ]; then
    echo "[✓] Virtual camera created at /dev/video0"
    
    # Show device info
    echo "[*] Device information:"
    v4l2-ctl --device=/dev/video0 --info
    
    echo ""
    echo "[*] Virtual camera is ready!"
    echo "[*] Use one of the following to feed test video:"
    echo ""
    echo "1. Moving ball (best for motion detection):"
    echo "   gst-launch-1.0 videotestsrc pattern=ball ! video/x-raw,width=640,height=480,framerate=30/1 ! v4l2sink device=/dev/video0"
    echo ""
    echo "2. Color bars (static):"
    echo "   gst-launch-1.0 videotestsrc pattern=smpte ! video/x-raw,width=640,height=480,framerate=30/1 ! v4l2sink device=/dev/video0"
    echo ""
    echo "3. Snow pattern (noise):"
    echo "   gst-launch-1.0 videotestsrc pattern=snow ! video/x-raw,width=640,height=480,framerate=30/1 ! v4l2sink device=/dev/video0"
    echo ""
    echo "4. Test motion detection script:"
    echo "   ./scripts/test_motion_detection.sh"
    
else
    echo "[✗] Failed to create virtual camera device"
    exit 1
fi
