#!/bin/bash

echo "[*] Test Video Generation Script"
echo "=============================="

OUTPUT_DIR="test_videos"
mkdir -p $OUTPUT_DIR

echo "[*] Generating test videos in $OUTPUT_DIR/"

# Test video 1: Moving ball (good for motion detection)
echo "[*] Generating moving_ball.mp4..."
gst-launch-1.0 videotestsrc pattern=ball num-buffers=300 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    x264enc bitrate=1000 ! mp4mux ! \
    filesink location=$OUTPUT_DIR/moving_ball.mp4 2>/dev/null

# Test video 2: Color bars (static)
echo "[*] Generating color_bars.mp4..."
gst-launch-1.0 videotestsrc pattern=smpte num-buffers=300 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    x264enc bitrate=1000 ! mp4mux ! \
    filesink location=$OUTPUT_DIR/color_bars.mp4 2>/dev/null

# Test video 3: Snow (noise)
echo "[*] Generating snow_noise.mp4..."
gst-launch-1.0 videotestsrc pattern=snow num-buffers=300 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    x264enc bitrate=1000 ! mp4mux ! \
    filesink location=$OUTPUT_DIR/snow_noise.mp4 2>/dev/null

# Test video 4: Alternating patterns (motion bursts)
echo "[*] Generating alternating_motion.mp4..."
gst-launch-1.0 videotestsrc pattern=black num-buffers=150 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    x264enc bitrate=1000 ! mp4mux ! \
    filesink location=$OUTPUT_DIR/alternating_part1.mp4 2>/dev/null

gst-launch-1.0 videotestsrc pattern=ball num-buffers=150 ! \
    video/x-raw,width=640,height=480,framerate=30/1 ! \
    x264enc bitrate=1000 ! mp4mux ! \
    filesink location=$OUTPUT_DIR/alternating_part2.mp4 2>/dev/null

# Combine alternating parts
echo "[*] Combining alternating patterns..."
if command -v ffmpeg >/dev/null 2>&1; then
    ffmpeg -i $OUTPUT_DIR/alternating_part1.mp4 -i $OUTPUT_DIR/alternating_part2.mp4 \
           -filter_complex "[0:v][1:v]concat=n=2:v=1:a=0[outv]" -map "[outv]" \
           $OUTPUT_DIR/alternating_motion.mp4 -y 2>/dev/null
    rm $OUTPUT_DIR/alternating_part1.mp4 $OUTPUT_DIR/alternating_part2.mp4
else
    echo "[!] ffmpeg not found, skipping combination"
fi

echo ""
echo "[✓] Test videos generated successfully!"
echo "[*] Available videos:"
ls -la $OUTPUT_DIR/
echo ""
echo "[*] Usage examples:"
echo "   1. Loop video for testing:"
echo "      gst-launch-1.0 filesrc location=$OUTPUT_DIR/moving_ball.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! v4l2sink device=/dev/video0"
echo ""
echo "   2. Stream via RTSP:"
echo "      gst-launch-1.0 filesrc location=$OUTPUT_DIR/moving_ball.mp4 ! qtdemux ! h264parse ! rtph264pay ! udpsink host=127.0.0.1 port=5000"
