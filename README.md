# Smart Monitor - IoT Baby Monitoring System

A comprehensive IoT baby monitoring system with real-time audio/video analysis, sensor integration, and web-based dashboard.

## 🏗️ Project Structure

```
smart-monitor/
├── src/                          # Source code
│   ├── core/                     # Core C/C++ functionality
│   │   ├── agent/                # Data agent and state machine
│   │   ├── audio/                # Audio processing
│   │   ├── video/                # Video processing
│   │   ├── sensors/              # Sensor interfaces
│   │   ├── communication/        # Communication protocols
│   │   ├── hardware/             # Hardware abstraction
│   │   ├── ffi/                 # Foreign function interface
│   │   └── common/              # Shared utilities
│   │
│   ├── rust/                    # Rust modules
│   │   ├── motion_detection/     # Motion detection algorithms
│   │   ├── image_processing/    # Image processing utilities
│   │   └── shared/             # Shared Rust utilities
│   │
│   └── web/                     # Web server code
│       ├── api/                 # HTTP API server
│       ├── protocol/            # Protocol server
│       └── webrtc/             # WebRTC functionality
│
├── web/                         # Web frontend
│   ├── static/                  # Static web files
│   │   ├── css/
│   │   ├── js/
│   │   └── index.html
│   └── assets/                  # Images, videos, etc.
│
├── docs/                        # Documentation
│   ├── api/                     # API documentation
│   ├── architecture/             # Architecture docs
│   └── user_guide/              # User documentation
│
├── scripts/                     # Build and utility scripts
│   ├── build/                   # Build scripts
│   ├── deploy/                  # Deployment scripts
│   ├── test/                    # Testing scripts
│   └── utils/                   # Utility scripts
│
├── config/                      # Configuration files
│   ├── cmake/                   # CMake modules
│   ├── docker/                  # Docker configurations
│   └── systemd/                 # Systemd service files
│
└── tests/                       # Test files
    ├── unit/
    ├── integration/
    └── fixtures/
```

## 🚀 Quick Start

### Prerequisites

- Linux system (Ubuntu 20.04+ recommended)
- CMake 3.16+
- GCC 9+ or Clang 10+
- Rust 1.70+ (if building Rust components)
- GStreamer 1.16+
- OpenCV 4.0+

### Build

```bash
# Clone the repository
git clone <repository-url>
cd smart-monitor

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DBUILD_RUST=ON -DBUILD_TESTS=ON

# Build
make -j$(nproc)

# Install (optional)
sudo make install
```

### Run

```bash
# Run the main application
./smart_monitor

# Or use the deployment script
../scripts/deploy/run_demo.sh
```

## 📋 Features

- **Real-time Audio Analysis**: Noise detection, voice activity, crying detection
- **Video Motion Detection**: Rust-based motion analysis with configurable thresholds
- **Multi-sensor Support**: Temperature, humidity, light, and motion sensors
- **Web Dashboard**: Real-time monitoring interface with live charts
- **RESTful API**: HTTP endpoints for integration with external systems
- **WebSocket Support**: Real-time bidirectional communication
- **Physics Simulation**: Realistic baby state machine with environmental correlation
- **Modular Architecture**: Clean separation of concerns with well-defined interfaces

## 🔧 Configuration

### System Configuration

Configuration files are located in `config/`:

- `config/systemd/smart-monitor.service` - Systemd service file
- `config/docker/` - Docker configurations
- `config/cmake/` - CMake modules

### Runtime Configuration

The system can be configured via:
- Command line arguments
- Configuration files
- Environment variables

## 📊 Architecture

The system consists of several interconnected components:

1. **Data Agent**: Central coordination and state management
2. **Audio Processor**: Real-time audio analysis and detection
3. **Video Processor**: Motion detection and video analysis
4. **Sensor Manager**: Environmental sensor integration
5. **Web Server**: HTTP API and WebSocket communication
6. **Dashboard**: Web-based monitoring interface

## 🧪 Testing

```bash
# Run unit tests
cd build && make test

# Run integration tests
./scripts/test/run_tests.sh

# Test motion detection
./scripts/test/test_motion_detection.sh
```

## 📚 Documentation

- **API Documentation**: `docs/api/`
- **Architecture Guide**: `docs/architecture/`
- **User Manual**: `docs/user_guide/`
- **Doxygen API**: Generate with `make docs` in build directory

## 🐳 Docker Support

```bash
# Build Docker image
docker build -t smart-monitor .

# Run with Docker
docker run -p 8083:8083 -p 8082:8082 smart-monitor
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🔗 Dependencies

- **GStreamer**: Multimedia framework
- **OpenCV**: Computer vision library
- **Rust**: Systems programming language
- **CMake**: Build system generator
- **pkg-config**: Package configuration helper

## 📞 Support

For support and questions:
- Create an issue in the repository
- Check the documentation in `docs/`
- Review the user guide in `docs/user_guide/`
