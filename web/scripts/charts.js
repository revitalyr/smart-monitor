// charts.js
// This module handles the initialization and updating of all Chart.js instances.

let charts = {}; // Global object to store chart instances

/**
 * Initializes all Chart.js charts on the dashboard.
 */
export function initCharts() {
    // IMU Chart
    const imuCtx = document.getElementById('imuChart').getContext('2d');
    charts.imu = new Chart(imuCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'X', data: [], borderColor: '#e74c3c', tension: 0.2 },
                { label: 'Y', data: [], borderColor: '#3498db', tension: 0.2 },
                { label: 'Z', data: [], borderColor: '#2ecc71', tension: 0.2 }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: { y: { beginAtZero: false } },
            plugins: { legend: { display: true } }
        }
    });
    
    // Audio Chart
    const audioCtx = document.getElementById('audioChart').getContext('2d');
    charts.audio = new Chart(audioCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'Amplitude', data: [], borderColor: '#9b59b6', fill: true }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: { y: { beginAtZero: true, max: 1.0 } },
            plugins: { legend: { display: false } }
        }
    });
    
    // Sleep Chart
    const sleepCtx = document.getElementById('sleepChart').getContext('2d');
    charts.sleep = new Chart(sleepCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'Sleep Quality %', data: [], borderColor: '#2ecc71', tension: 0.4 }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: { y: { beginAtZero: true, max: 100 } },
            plugins: { legend: { display: false } }
        }
    });
    
    // Helper for Gauge charts (Doughnut chart with specific options)
    const initGauge = (id, color) => {
        return new Chart(document.getElementById(id).getContext('2d'), {
            type: 'doughnut',
            data: {
                datasets: [{
                    data: [0, 100],
                    backgroundColor: [color, '#eee'],
                    borderWidth: 0
                }]
            },
            options: {
                circumference: 180, // Half circle
                rotation: 270,      // Start from bottom
                cutout: '80%',      // Thickness of the ring
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });
    };

    charts.cpu = initGauge('cpuGauge', '#e74c3c');
    charts.ram = initGauge('ramGauge', '#3498db');
    charts.fpsGauge = initGauge('fpsGauge', '#f1c40f');

    // FPS Line Chart
    const fpsCtx = document.getElementById('fpsChart').getContext('2d');
    charts.fps = new Chart(fpsCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'FPS',
                data: [],
                borderColor: '#f1c40f',
                backgroundColor: 'rgba(241, 196, 15, 0.1)',
                fill: true,
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: { y: { beginAtZero: true, suggestedMax: 35 }, x: { display: false } },
            plugins: { legend: { display: false } }
        }
    });

    // System Metrics Bar Chart
    const metricsCtx = document.getElementById('metricsChart').getContext('2d');
    charts.metrics = new Chart(metricsCtx, {
        type: 'bar',
        data: {
            labels: ['Events', 'Frames (k)', 'CPU', 'Mem', 'Latency (ms)'],
            datasets: [{
                label: 'Current',
                data: [0, 0, 0, 0, 0],
                backgroundColor: ['#e74c3c', '#3498db', '#f39c12', '#2ecc71', '#9b59b6']
            }]
        },
        options: { responsive: true, maintainAspectRatio: false, plugins: { legend: { display: false } } }
    });
}

/**
 * Updates the IMU chart with new data.
 * @param {object} data - Object containing acc_x, acc_y, acc_z.
 */
export function updateIMU(data) {
    if (!data || !charts.imu) return;
    const chart = charts.imu;
    chart.data.labels.push('');
    chart.data.datasets[0].data.push(data.acc_x || 0);
    chart.data.datasets[1].data.push(data.acc_y || 0);
    chart.data.datasets[2].data.push(data.acc_z || 0);
    
    if (chart.data.labels.length > 20) { // Keep last 20 points
        chart.data.labels.shift();
        chart.data.datasets.forEach(ds => ds.data.shift());
    }
    chart.update('none');
}

/**
 * Updates the Audio chart with new data and shows alerts.
 * @param {object} data - Object containing audio_level and alert status.
 */
export function updateAudio(data) {
    if (!data || !charts.audio) return;
    const chart = charts.audio;
    chart.data.labels.push('');
    chart.data.datasets[0].data.push(data.audio_level || 0);
    
    if (chart.data.labels.length > 20) { // Keep last 20 points
        chart.data.labels.shift();
        chart.data.datasets.forEach(ds => ds.data.shift());
    }
    chart.update('none');
    
    if (data.alert) {
        // Assuming showAlert is available in the global scope or imported
        // For now, let's just log it if showAlert is not imported here.
        console.log('🚨 Baby Crying Detected!');
    }
}

// Export the charts object for direct access if needed (e.g., for debugging)
export { charts };