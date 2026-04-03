// Smart Monitor Application JavaScript

// Global variables
let isAnalyzing = false;
let dataChannel = null;

// Initialize application
document.addEventListener('DOMContentLoaded', function() {
    console.log('Smart Monitor application initialized');
    
    // Setup navigation
    setupNavigation();
    
    // Setup charts
    setupCharts();
    
    // Start data channel
    startDataChannel();
});

function setupNavigation() {
    const navButtons = document.querySelectorAll('.nav-btn');
    navButtons.forEach(button => {
        button.addEventListener('click', function() {
            const page = this.getAttribute('data-page');
            showPage(page);
        });
    });
}

function showPage(pageName) {
    // Hide all pages
    const pages = document.querySelectorAll('.page');
    pages.forEach(page => page.classList.remove('active'));
    
    // Show selected page
    const targetPage = document.getElementById(pageName + '-page');
    if (targetPage) {
        targetPage.classList.add('active');
    }
    
    // Update navigation
    const navButtons = document.querySelectorAll('.nav-btn');
    navButtons.forEach(btn => btn.classList.remove('active'));
    
    const activeButton = document.querySelector(`[data-page="${pageName}"]`);
    if (activeButton) {
        activeButton.classList.add('active');
    }
}

function setupCharts() {
    // Chart.js setup would go here
    console.log('Charts initialized');
}

function startDataChannel() {
    if (dataChannel) return;
    
    dataChannel = {
        interval: null,
        start: function() {
            this.interval = setInterval(updateData, 1000);
        },
        stop: function() {
            if (this.interval) {
                clearInterval(this.interval);
                this.interval = null;
            }
        }
    };
    
    dataChannel.start();
}

function updateData() {
    // Fetch sensor data
    fetch('/sensors/i2c')
        .then(response => response.json())
        .then(data => {
            updateSensorDisplay(data);
        })
        .catch(error => {
            console.error('Error fetching sensor data:', error);
        });
    
    // Fetch audio data
    fetch('/audio/status')
        .then(response => response.json())
        .then(data => {
            updateAudioDisplay(data);
        })
        .catch(error => {
            console.error('Error fetching audio data:', error);
        });
}

function updateSensorDisplay(data) {
    const tempElement = document.getElementById('temperature');
    const humidityElement = document.getElementById('humidity');
    const lightElement = document.getElementById('light');
    const motionElement = document.getElementById('motion');
    
    if (tempElement) tempElement.textContent = data.temperature.toFixed(1) + '°C';
    if (humidityElement) humidityElement.textContent = data.humidity.toFixed(1) + '%';
    if (lightElement) lightElement.textContent = data.light;
    if (motionElement) motionElement.textContent = data.motion ? 'YES' : 'NO';
}

function updateAudioDisplay(data) {
    const noiseElement = document.getElementById('noiseLevel');
    const voiceElement = document.getElementById('voiceActivity');
    const cryingElement = document.getElementById('babyCryingStatus');
    const screamingElement = document.getElementById('screamingStatus');
    
    if (noiseElement) noiseElement.textContent = data.noise_level.toFixed(3);
    if (voiceElement) voiceElement.textContent = data.voice_activity ? 'ACTIVE' : 'QUIET';
    if (cryingElement) cryingElement.textContent = data.baby_crying ? 'YES' : 'NO';
    if (screamingElement) screamingElement.textContent = data.screaming ? 'YES' : 'NO';
}

// Global functions for button clicks
window.startAnalysis = function() {
    isAnalyzing = true;
    console.log('Analysis started');
    
    fetch('/analyze/video', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: 'mock' })
    })
    .then(response => response.json())
    .then(data => {
        console.log('Analysis response:', data);
    })
    .catch(error => {
        console.error('Analysis error:', error);
    });
};

window.stopAnalysis = function() {
    isAnalyzing = false;
    console.log('Analysis stopped');
};

window.takeSnapshot = function() {
    console.log('Snapshot taken');
};

window.toggleRecording = function() {
    console.log('Recording toggled');
};
