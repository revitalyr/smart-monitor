#!/bin/bash

echo "[*] Building Smart Monitor (C version)"
echo "====================================="

# Check dependencies
echo "[*] Checking dependencies..."
command -v cmake >/dev/null 2>&1 || { echo "CMake is required but not installed."; exit 1; }
command -v cargo >/dev/null 2>&1 || { echo "Rust/Cargo is required but not installed."; exit 1; }
command -v pkg-config >/dev/null 2>&1 || { echo "pkg-config is required but not installed."; exit 1; }

# Check for GStreamer development packages
echo "[*] Checking GStreamer development packages..."
pkg-config --exists gstreamer-1.0 || { echo "gstreamer-1.0 development packages are required."; exit 1; }
pkg-config --exists gstreamer-app-1.0 || { echo "gstreamer-app-1.0 development packages are required."; exit 1; }
pkg-config --exists gstreamer-video-1.0 || { echo "gstreamer-video-1.0 development packages are required."; exit 1; }

# Build Rust module
echo "[*] Building Rust motion detection module..."
cd firmware/rust_modules/motion && cargo build --release
if [ $? -ne 0 ]; then
    echo "[!] Rust build failed"
    exit 1
fi
cd ../../..

# Build C application
echo "[*] Building C application..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "[!] CMake configuration failed"
    exit 1
fi

cmake --build build --parallel
if [ $? -ne 0 ]; then
    echo "[!] Build failed"
    exit 1
fi

echo "[✓] Build complete!"
echo "[*] Binary location: ./build/firmware/c_core/smart_monitor"
echo "[*] To run: ./build/firmware/c_core/smart_monitor"
echo "[*] Dashboard will be available at: http://localhost:8080"
