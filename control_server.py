#!/usr/bin/env python3
"""
Control server for Smart Monitor with web interface
"""

import subprocess
import threading
import json
import time
from flask import Flask, render_template_string, jsonify, request
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

smart_monitor_process = None
monitoring_active = False

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Smart Monitor Control</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        .control-panel { background: #f5f5f5; padding: 20px; border-radius: 5px; margin: 20px 0; }
        .status-panel { background: #e9ecef; padding: 20px; border-radius: 5px; margin: 20px 0; }
        .btn { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        .btn:hover { background: #0056b3; }
        .btn.stop { background: #dc3545; }
        .btn.stop:hover { background: #c82333; }
        .btn:disabled { background: #6c757d; cursor: not-allowed; }
        input[type="text"] { width: 70%; padding: 8px; margin: 5px; }
        .metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .metric { background: white; padding: 10px; border-radius: 3px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Monitor Control Panel</h1>
        
        <div class="control-panel">
            <h2>Video Analysis Control</h2>
            <input type="text" id="videoUrl" value="https://interactive-examples.mdn.mozilla.net/media/cc0-videos/flower.mp4" placeholder="Enter video URL">
            <br>
            <button id="startBtn" class="btn" onclick="startMonitoring()">Start Analysis</button>
            <button id="stopBtn" class="btn stop" onclick="stopMonitoring()" disabled>Stop Analysis</button>
            <div id="controlStatus" style="margin-top: 10px;"></div>
        </div>
        
        <div class="status-panel">
            <h2>Real-time Analysis Results</h2>
            <div class="metrics">
                <div class="metric">
                    <strong>Status:</strong> <span id="status">Inactive</span>
                </div>
                <div class="metric">
                    <strong>Motion Events:</strong> <span id="motionEvents">0</span>
                </div>
                <div class="metric">
                    <strong>Motion Level:</strong> <span id="motionLevel">0.00</span>
                </div>
                <div class="metric">
                    <strong>Frames Processed:</strong> <span id="framesProcessed">0</span>
                </div>
            </div>
        </div>
    </div>

    <script>
        let monitoringActive = false;

        async function startMonitoring() {
            const url = document.getElementById('videoUrl').value;
            const startBtn = document.getElementById('startBtn');
            const stopBtn = document.getElementById('stopBtn');
            const status = document.getElementById('controlStatus');
            
            startBtn.disabled = true;
            status.innerHTML = '<p style="color: blue;">Starting Smart Monitor...</p>';
            
            try {
                const response = await fetch('/start', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ url: url })
                });
                
                const result = await response.json();
                if (response.ok) {
                    monitoringActive = true;
                    startBtn.disabled = true;
                    stopBtn.disabled = false;
                    status.innerHTML = '<p style="color: green;">Smart Monitor started successfully</p>';
                } else {
                    status.innerHTML = `<p style="color: red;">Error: ${result.error}</p>`;
                    startBtn.disabled = false;
                }
            } catch (error) {
                status.innerHTML = `<p style="color: red;">Network error: ${error.message}</p>`;
                startBtn.disabled = false;
            }
        }

        async function stopMonitoring() {
            const startBtn = document.getElementById('startBtn');
            const stopBtn = document.getElementById('stopBtn');
            const status = document.getElementById('controlStatus');
            
            try {
                const response = await fetch('/stop', { method: 'POST' });
                const result = await response.json();
                
                monitoringActive = false;
                startBtn.disabled = false;
                stopBtn.disabled = true;
                status.innerHTML = '<p style="color: orange;">Smart Monitor stopped</p>';
            } catch (error) {
                status.innerHTML = `<p style="color: red;">Error stopping: ${error.message}</p>`;
            }
        }

        async function updateMetrics() {
            if (!monitoringActive) return;
            
            try {
                const response = await fetch('/metrics');
                const data = await response.json();
                
                document.getElementById('status').textContent = data.status || 'Active';
                document.getElementById('motionEvents').textContent = data.motion_events || 0;
                document.getElementById('motionLevel').textContent = (data.motion_level || 0).toFixed(2);
                document.getElementById('framesProcessed').textContent = data.frames_processed || 0;
            } catch (error) {
                console.error('Failed to update metrics:', error);
            }
        }

        // Update metrics every second when monitoring is active
        setInterval(updateMetrics, 1000);
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return HTML_TEMPLATE

@app.route('/start', methods=['POST'])
def start_monitoring():
    global smart_monitor_process, monitoring_active
    
    if monitoring_active:
        return jsonify({"error": "Smart Monitor is already running"})
    
    try:
        data = request.json
        video_url = data.get('url', '')
        
        # Start smart_monitor process
        smart_monitor_process = subprocess.Popen([
            '/mnt/d/work/smart-monitor/build/firmware/cpp_core/smart_monitor'
        ], cwd='/mnt/d/work/smart-monitor')
        
        monitoring_active = True
        return jsonify({
            "status": "started",
            "video_url": video_url,
            "message": "Smart Monitor started successfully"
        })
    except Exception as e:
        return jsonify({"error": str(e)})

@app.route('/stop', methods=['POST'])
def stop_monitoring():
    global smart_monitor_process, monitoring_active
    
    if not monitoring_active:
        return jsonify({"error": "Smart Monitor is not running"})
    
    try:
        if smart_monitor_process:
            smart_monitor_process.terminate()
            smart_monitor_process.wait()
            smart_monitor_process = None
        
        monitoring_active = False
        return jsonify({
            "status": "stopped",
            "message": "Smart Monitor stopped successfully"
        })
    except Exception as e:
        return jsonify({"error": str(e)})

@app.route('/metrics')
def get_metrics():
    try:
        import requests
        response = requests.get('http://localhost:8080/metrics', timeout=1)
        if response.status_code == 200:
            return response.json()
    except:
        pass
    
    # Return default values if smart_monitor is not accessible
    return {
        "status": "inactive" if not monitoring_active else "active",
        "motion_events": 0,
        "motion_level": 0.0,
        "frames_processed": 0
    }

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
