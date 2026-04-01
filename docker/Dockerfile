FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV GST_DEBUG=3
ENV RUST_LOG=info

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install GStreamer and plugins
RUN apt-get update && apt-get install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    libgstreamer-app1.0-dev \
    libgstreamer-video1.0-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Install video4linux development headers
RUN apt-get update && apt-get install -y \
    libv4l-dev \
    v4l-utils \
    && rm -rf /var/lib/apt/lists/*

# Create application directory
WORKDIR /app

# Copy source code
COPY . .

# Build Rust motion detection module
RUN cd firmware/rust_modules/motion && cargo build --release

# Build C++ application
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
RUN cmake --build build --parallel

# Install application
RUN cmake --build build --target install

# Create non-root user for security
RUN useradd -m -s /bin/bash smartmonitor && \
    usermod -aG video smartmonitor

# Copy configuration files
RUN mkdir -p /etc/smartmonitor && \
    cp linux/configs/smart-monitor.conf /etc/smart-monitor.conf

# Copy systemd service file (for reference)
RUN mkdir -p /etc/systemd/system && \
    cp linux/systemd/smart-monitor.service /etc/systemd/system/

# Copy web dashboard
RUN mkdir -p /var/www/smart-monitor && \
    cp -r web/dashboard/* /var/www/smart-monitor/

# Set permissions
RUN chown -R smartmonitor:smartmonitor /var/www/smart-monitor && \
    chown smartmonitor:smartmonitor /usr/local/bin/smart_monitor

# Expose ports
EXPOSE 8080
EXPOSE 10000-20000/udp

# Switch to non-root user
USER smartmonitor

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Start the application
CMD ["/usr/local/bin/smart_monitor"]
