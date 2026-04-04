#!/bin/bash

# Smart Monitor Full Demo Build Script
# Builds with all features enabled for complete demonstration

set -e

echo "🚀 Building Smart Monitor with Full Feature Set..."
echo "=================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for basic build tools
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Please install: sudo apt install cmake"
        exit 1
    fi
    
    if ! command -v gcc &> /dev/null; then
        print_error "GCC not found. Please install: sudo apt install build-essential"
        exit 1
    fi
    
    if ! command -v cargo &> /dev/null; then
        print_error "Rust/Cargo not found. Please install: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh"
        exit 1
    fi
    
    print_status "✅ Basic dependencies found"
}

# Build Rust motion detection module
build_rust_module() {
    print_status "Building Rust motion detection module..."
    cd firmware/rust_modules/motion
    
    if cargo build --release; then
        print_status "✅ Rust module built successfully"
    else
        print_error "❌ Rust module build failed"
        exit 1
    fi
    
    cd ../../..
}

# Build C application with all features
build_c_application() {
    print_status "Building C application with full feature set..."
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure with all features enabled
    print_status "Configuring CMake with all features..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_MOCK=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_SENSORS=ON \
        -DENABLE_WEBRTC=ON \
        -DWITH_SYSTEMD=ON \
        -DENABLE_CURL=ON
    
    # Build
    print_status "Compiling application..."
    if ninja smart_monitor; then
        print_status "✅ C application built successfully"
    else
        print_error "❌ C application build failed"
        exit 1
    fi
    
    cd ..
}

# Create configuration file
create_config() {
    print_status "Creating configuration file..."
    
    mkdir -p config
    cat > config/smartmonitor.conf << EOF
# Smart Monitor Full Demo Configuration
device=/dev/video0
mock_mode=true
port=8080
enable_audio=true
enable_sensors=true
enable_webrtc=false
debug=true
log_file=/tmp/smartmonitor.log
i2c_device=/dev/i2c-1
i2c_address=0x48
sensor_poll_interval=1000
uart_device=/dev/ttyUSB0
uart_baudrate=115200
esp32_uart=/dev/ttyUSB1
noise_detection=true
baby_crying_threshold=0.3
screaming_threshold=0.5
sleep_score_enabled=true
EOF
    
    print_status "✅ Configuration file created"
}

# Create systemd service file
create_systemd_service() {
    print_status "Creating systemd service file..."
    
    mkdir -p systemd
    cat > systemd/smartmonitor.service << EOF
[Unit]
Description=SmartMonitor - Embedded Video Analytics System
After=network.target sound.target

[Service]
Type=simple
ExecStart=$(pwd)/build/firmware/c_core/smart_monitor --config $(pwd)/config/smartmonitor.conf
Restart=always
RestartSec=5
User=root
Environment=SMART_MONITOR_CONFIG=$(pwd)/config/smartmonitor.conf
Environment=SMART_MONITOR_LOG_LEVEL=INFO

[Install]
WantedBy=multi-user.target
EOF
    
    print_status "✅ Systemd service file created"
}

# Create demo script
create_demo_script() {
    print_status "Creating demo script..."
    
    cat > scripts/run_demo.sh << 'EOF'
#!/bin/bash

# Smart Monitor Demo Runner

echo "🎬 Starting Smart Monitor Full Demo..."
echo "===================================="

# Set up environment
export SMART_MONITOR_CONFIG="$(pwd)/config/smartmonitor.conf"
export SMART_MONITOR_LOG_LEVEL="INFO"

# Check if built
if [ ! -f "build/firmware/c_core/smart_monitor" ]; then
    echo "❌ Application not built. Run ./build_full_demo.sh first"
    exit 1
fi

echo "🔧 Configuration: $SMART_MONITOR_CONFIG"
echo "📊 Log file: /tmp/smartmonitor.log"
echo "🌐 Web interface: http://localhost:8080"
echo ""
echo "📋 Features enabled:"
echo "  ✅ Video motion detection (Rust)"
echo "  ✅ Audio capture & noise detection"
echo "  ✅ Baby crying detection"
echo "  ✅ I2C sensor simulation"
echo "  ✅ SPI IMU device"
echo "  ✅ UART communication"
echo "  ✅ ESP32 device simulation"
echo "  ✅ Sleep score calculation"
echo ""
echo "🎯 Starting application..."
echo "Press Ctrl+C to stop"
echo ""

# Run the application
./build/firmware/c_core/smart_monitor --config "$SMART_MONITOR_CONFIG"
EOF

    chmod +x scripts/run_demo.sh
    print_status "✅ Demo script created"
}

# Main execution
main() {
    print_status "Starting full demo build process..."
    
    check_dependencies
    build_rust_module
    build_c_application
    create_config
    create_systemd_service
    create_demo_script
    
    echo ""
    print_status "🎉 Full demo build completed successfully!"
    echo ""
    echo "📋 What was built:"
    echo "  ✅ Rust motion detection module"
    echo "  ✅ C application with all features"
    echo "  ✅ Configuration file"
    echo "  ✅ Systemd service"
    echo "  ✅ Demo runner script"
    echo ""
    echo "🚀 To run the demo:"
    echo "  ./scripts/run_demo.sh"
    echo ""
    echo "🌐 Web interface will be available at:"
    echo "  http://localhost:8080"
    echo ""
    echo "📊 Features demonstrated:"
    echo "  📹 Video motion detection"
    echo "  🎵 Audio capture & analysis"
    echo "  👶 Baby crying detection"
    echo "  🌡️ I2C temperature/humidity sensors"
    echo "  📱 SPI IMU (accelerometer/gyroscope)"
    echo "  📡 UART communication"
    echo "  📶 ESP32 device simulation"
    echo "  😴 Sleep score calculation"
    echo ""
    print_status "Ready for full embedded IoT demonstration! 🚀"
}

# Run main function
main "$@"
