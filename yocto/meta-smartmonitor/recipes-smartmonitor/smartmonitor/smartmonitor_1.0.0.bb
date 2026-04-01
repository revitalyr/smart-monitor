SUMMARY = "SmartMonitor - Embedded Video Analytics System"
DESCRIPTION = "Production-ready embedded video monitoring system with Rust-based motion detection, WebRTC streaming, and real-time metrics"
HOMEPAGE = "https://github.com/smart-monitor"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=0835ade698e0bcf286f5f9da5bd7a5eb"

SECTION = "applications"

DEPENDS = " \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    alsa-lib \
    libv4l \
    rust-native \
    systemd \
"

SRC_URI = "git://github.com/smart-monitor/smart-monitor.git;branch=master;protocol=https"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

# Rust cross-compilation setup
CARGO_TARGET = "aarch64-unknown-linux-gnu"
CARGO_BUILD_FLAGS = "--release --target ${CARGO_TARGET}"

inherit cmake systemd pkgconfig

# Override CMake toolchain for cross-compilation
OECMAKE_GENERATOR = "Ninja"
OECMAKE_TOOLCHAIN_FILE = "${WORKDIR}/cmake/arm64-toolchain.cmake"

# Package configuration
PACKAGECONFIG ??= "mock systemd"
PACKAGECONFIG[mock] = "--enable-mock,--disable-mock"
PACKAGECONFIG[systemd] = "--with-systemd,--without-systemd,systemd"

# Systemd service
SYSTEMD_SERVICE_${PN} = "smartmonitor.service"
SYSTEMD_AUTO_ENABLE_${PN} = "enable"

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DENABLE_MOCK=${@bb.utils.contains('PACKAGECONFIG', 'mock', 'ON', 'OFF')} \
    -DWITH_SYSTEMD=${@bb.utils.contains('PACKAGECONFIG', 'systemd', 'ON', 'OFF')} \
"

# Install directories
bindir = "${bindir}"
sbindir = "${sbindir}"
libexecdir = "${libexecdir}"
sysconfdir = "${sysconfdir}"
localstatedir = "${localstatedir}"
libdir = "${libdir}"
includedir = "${includedir}"
datadir = "${datadir}"

# Configuration files
do_install_append() {
    install -d ${D}${sysconfdir}
    install -m 0644 ${S}/config/smartmonitor.conf ${D}${sysconfdir}/
    
    # Install systemd service
    if ${@bb.utils.contains('PACKAGECONFIG', 'systemd', 'true', 'false')}; then
        install -d ${D}${systemd_system_unitdir}
        install -m 0644 ${S}/systemd/smartmonitor.service ${D}${systemd_system_unitdir}/
    fi
    
    # Install udev rules for camera access
    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${S}/udev/99-smartmonitor.rules ${D}${sysconfdir}/udev/rules.d/
}

# Package content
FILES_${PN} += " \
    ${bindir}/smart_monitor \
    ${sysconfdir}/smartmonitor.conf \
    ${sysconfdir}/udev/rules.d/99-smartmonitor.rules \
"

FILES_${PN}-dev += " \
    ${includedir} \
    ${libdir}/*.so \
"

FILES_${PN}-dbg += " \
    ${bindir}/.debug \
    ${libdir}/.debug \
"

CONFFILES_${PN} = "${sysconfdir}/smartmonitor.conf"

# Systemd integration
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "smartmonitor.service"

# Runtime dependencies
RDEPENDS_${PN} = " \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    alsa-lib \
    libv4l \
    bash \
"

# Build-time dependencies
BDEPENDS = " \
    cmake-native \
    ninja-native \
    rust-native \
    pkgconfig-native \
"

# Package metadata
DESCRIPTION_${PN} = "SmartMonitor embedded video analytics system with motion detection"
DESCRIPTION_${PN}-dev = "Development files for SmartMonitor"

# QA checks
INSANE_SKIP_${PN} += "dev-so"
INSANE_SKIP_${PN}-dev += "ldflags"

# Security
SECURITY_CFLAGS = "${SECURITY_NO_PIE_CFLAGS}"
SECURITY_LDFLAGS = "${SECURITY_NO_PIE_LDFLAGS}"

# Performance optimization
TARGET_CC_ARCH = "aarch64"
TARGET_CFLAGS = "-O2 -mcpu=cortex-a53"
TARGET_LDFLAGS = "-Wl,-O1"
