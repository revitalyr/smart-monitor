# Enable additional features for SmartMonitor
PACKAGECONFIG_append = " webrtc audio"

# Additional dependencies for full feature set
DEPENDS_append = " \
    webrtc-native \
    pulseaudio \
    json-c \
    curl \
"

# Enable WebRTC and audio processing in CMake
EXTRA_OECMAKE_append = " \
    -DENABLE_WEBRTC=ON \
    -DENABLE_AUDIO=ON \
    -DENABLE_CURL=ON \
"
