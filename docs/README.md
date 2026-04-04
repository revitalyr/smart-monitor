# Smart Monitor Technical Documentation

This directory contains interactive technical documentation for the Smart Monitor IoT baby monitoring system.

## Files

- **index.html** - Main documentation page with interactive Mermaid diagrams
- **mermaid-height-util.js** - Utility for diagram scaling and height adjustment
- **README.md** - This file

## Features

### Interactive Documentation
- **Tabbed Navigation**: Navigate between Overview, Architecture, API, Data Flow, and Deployment sections
- **Mermaid Diagrams**: Visual representations of system components and interactions
- **Responsive Design**: Works on desktop and mobile devices
- **Zoom Controls**: Scale diagrams from 50% to 200% with smooth transitions

### Sections

1. **📊 Overview** - System architecture and key features
2. **🏗️ Architecture** - Component interaction and data flow
3. **🔄 API** - HTTP endpoints and request/response patterns
4. **📈 Data Flow** - Processing pipeline and state transitions
5. **🚀 Deployment** - Build system and runtime requirements

### Technology Stack

- **Core**: C/C++ with Rust integration
- **Frontend**: HTML5, CSS3, JavaScript
- **Visualization**: Mermaid.js diagrams
- **Real-time**: WebSocket and HTTP protocols
- **Processing**: GStreamer (audio), OpenCV (video)
- **Simulation**: Physics-based baby state machine

## Usage

### Local Viewing
Open `index.html` in a web browser. An internet connection is required for loading Mermaid.js from CDN.

### Diagram Controls
- **Zoom**: Use `+`/`−` buttons to scale diagrams
- **Reset**: Click the `⟲` button to reset to 100%
- **Navigation**: Click tabs to switch between sections

### Browser Compatibility
- Chrome 80+
- Firefox 75+
- Safari 13+
- Edge 80+

## Documentation Structure

The documentation provides comprehensive coverage of:

- System overview and capabilities
- Component architecture and interactions
- API endpoints and data formats
- Data processing pipelines
- Deployment and operational procedures

Each section includes interactive diagrams that illustrate the concepts visually, making complex technical information more accessible and easier to understand.

## Development

The documentation is designed to be:
- **Self-contained**: Single HTML file with embedded styles
- **Interactive**: Dynamic diagram rendering and controls
- **Maintainable**: Clear separation of concerns
- **Accessible**: Semantic HTML and responsive design

For questions or contributions, refer to the main project repository.
