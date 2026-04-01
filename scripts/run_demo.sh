#!/bin/bash

echo "[*] Smart Monitor Demo Setup"
echo "================================"

# Check dependencies
echo "[*] Checking dependencies..."
command -v cmake >/dev/null 2>&1 || { echo "CMake is required but not installed."; exit 1; }
command -v cargo >/dev/null 2>&1 || { echo "Rust/Cargo is required but not installed."; exit 1; }
command -v pkg-config >/dev/null 2>&1 || { echo "pkg-config is required but not installed."; exit 1; }

# Build Rust module
echo "[*] Building Rust motion detection module..."
cd firmware/rust_modules/motion && cargo build --release
cd ../../..

# Build C++ application
echo "[*] Building C++ application..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

echo "[*] Build complete!"
echo "[*] Starting Smart Monitor..."
echo "[*] Dashboard will be available at: http://localhost:8080"
echo "[*] Press Ctrl+C to stop"

# Run the application
./build/firmware/cpp_core/smart_monitor
