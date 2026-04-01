# Smart Monitor

A production-level embedded video monitoring system with real-time motion detection, WebRTC streaming, and modern web dashboard.

## 🚀 Features

- **Embedded Linux**: V4L2 camera integration with GStreamer pipeline
- **Rust Processing**: Safe motion detection via FFI integration
- **Real-time Streaming**: WebRTC live video streaming
- **REST API**: Health, metrics, and control endpoints
- **Modern Dashboard**: Responsive web UI with live charts
- **Production Ready**: systemd service, Docker containerization
- **Zero-copy**: Optimized video pipeline with DMA support

## 📋 System Requirements

### Development
- CMake 3.16+
- GCC/Clang with C11 support
- Rust 1.70+
- GStreamer 1.16+ with plugins
- libv4l-dev
- pkg-config

### Runtime
- Linux with kernel 4.15+
- USB camera (V4L2 compatible)
- Network connectivity for WebRTC

## 🛠️ Installation

### From Source (C Version)

```bash
# Clone repository
git clone <repository-url>
cd smart-monitor

# Build Rust module
cd firmware/rust_modules/motion
cargo build --release
cd ../../..

# Build C application
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run application
./build/firmware/c_core/smart_monitor
```

### Quick Demo (C Version)

```bash
# Run C build script
./scripts/build_c.sh
```

### Docker Deployment

```bash
# Build and run with Docker Compose
cd docker
docker-compose up --build

# Or build standalone image
docker build -t smart-monitor .
docker run -p 8080:8080 --device /dev/video0:/dev/video0 smart-monitor
```

## 🎯 Usage

### Web Dashboard
Open `http://localhost:8080` in your browser to access the web dashboard.

### REST API Endpoints

- `GET /health` - Service health status
- `GET /metrics` - Real-time metrics and motion data
- `GET /webrtc/offer` - WebRTC SDP offer
- `POST /webrtc/answer` - WebRTC SDP answer

### Configuration

Edit `/etc/smart-monitor.conf` to customize:

```ini
[video]
device=/dev/video0
width=640
height=480
fps=30

[motion]
threshold=20
motion_ratio_threshold=0.05

[network]
http_port=8080
```

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   V4L2 Camera   │───▶│ GStreamer Pipe   │───▶│  Motion Detect  │
│   /dev/video0   │    │   (H264/RTSP)    │    │   (Rust FFI)    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │                        │
                                ▼                        ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   WebRTC Stream │◀───│   REST API       │◀───│   Metrics       │
│   (Live Video)  │    │   (HTTP/HTTPS)   │    │   Engine        │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │
                                ▼
                       ┌──────────────────┐
                       │   Web Dashboard  │
                       │   (React/Charts) │
                       └──────────────────┘
```

## 📊 Metrics

The system provides real-time metrics including:

- Motion events count and level
- Frames processed per second
- Camera status and uptime
- Network streaming statistics

## 🔧 Development

### Project Structure

```
smart-monitor/
├── firmware/
│   ├── c_core/              # C application core
│   ├── rust_modules/        # Rust processing modules
│   └── drivers/            # Hardware interface drivers
├── web/dashboard/          # Web frontend
├── linux/
│   ├── systemd/           # systemd service files
│   └── configs/           # Configuration files
├── docker/               # Containerization
├── scripts/              # Build and demo scripts
└── docs/                # Documentation
```

### Building for ARM (Cross-compilation)

```bash
# Install ARM toolchain
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Cross-compile
cmake -B build-arm \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm
```

### Testing

```bash
# Run Rust tests
cd firmware/rust_modules/motion
cargo test

# Run C++ tests (if available)
cd build
ctest
```

## 🚀 Deployment

### Systemd Service

```bash
# Install service
sudo cp linux/systemd/smart-monitor.service /etc/systemd/system/
sudo cp linux/configs/smart-monitor.conf /etc/

# Enable and start
sudo systemctl enable smart-monitor
sudo systemctl start smart-monitor

# Check status
sudo systemctl status smart-monitor
```

### Production Considerations

- **Security**: Enable HTTPS with TLS certificates
- **Performance**: Configure zero-copy DMA buffers
- **Monitoring**: Set up log rotation and health checks
- **Scaling**: Use Kubernetes or Docker Swarm for multi-node deployment

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- GStreamer for multimedia processing
- cpp-httplib for HTTP server functionality
- Chart.js for dashboard visualizations
- Rust community for safe systems programming

## 📞 Support

For support and questions:

- Create an issue on GitHub
- Check the documentation in `/docs`
- Review the configuration examples in `/examples`
