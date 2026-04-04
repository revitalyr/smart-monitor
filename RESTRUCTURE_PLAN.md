# Directory Restructuring Plan

## Current Structure Issues

### Problems:
1. **Mixed concerns**: `web/` contains both server code and static assets
2. **Inconsistent naming**: `c_core/` vs `rust_modules/`
3. **Scattered scripts**: Scripts in multiple locations
4. **Unclear separation**: `firmware/` contains both core and drivers
5. **Missing logical grouping**: No clear separation by functionality

## Proposed New Structure

```
smart-monitor/
├── README.md
├── CMakeLists.txt
├── .gitignore
├── .gitmodules
│
├── src/                          # Source code (instead of firmware/)
│   ├── core/                     # Core C/C++ code (instead of c_core/)
│   │   ├── agent/                # Data agent and state machine
│   │   ├── audio/                # Audio processing
│   │   ├── video/                # Video processing
│   │   ├── sensors/              # Sensor interfaces
│   │   ├── communication/        # Communication protocols (instead of comm/)
│   │   ├── hardware/             # Hardware abstraction (instead of esp32/)
│   │   ├── ffi/                 # Foreign function interface
│   │   └── common/              # Shared utilities and constants
│   │
│   ├── rust/                    # Rust modules (instead of rust_modules/)
│   │   ├── motion_detection/     # Motion detection algorithms
│   │   ├── image_processing/    # Image processing utilities
│   │   └── shared/             # Shared Rust utilities
│   │
│   ├── web/                     # Web server code
│   │   ├── api/                 # HTTP API server
│   │   │   ├── http_server.c
│   │   │   └── http_server.h
│   │   ├── protocol/            # Protocol server
│   │   │   ├── protocol_server.c
│   │   │   └── protocol_server.h
│   │   └── webrtc/             # WebRTC functionality
│   │       ├── webrtc_server.c
│   │       └── webrtc_server.h
│   │
│   └── drivers/                 # Hardware drivers
│       ├── i2c/
│       ├── spi/
│       └── camera/
│
├── web/                         # Web frontend (static assets only)
│   ├── static/                  # Static web files
│   │   ├── css/
│   │   ├── js/
│   │   └── index.html
│   └── assets/                  # Images, videos, etc.
│
├── docs/                        # Documentation
│   ├── api/                     # API documentation
│   ├── architecture/             # Architecture docs
│   ├── deployment/              # Deployment guides
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
├── external/                     # Third-party dependencies
├── build/                       # Build output
├── target/                      # Build artifacts
└── tests/                       # Test files
    ├── unit/
    ├── integration/
    └── fixtures/
```

## Migration Steps

### Phase 1: Core Structure
1. Create new `src/` directory
2. Move `firmware/c_core/` → `src/core/`
3. Move `firmware/rust_modules/` → `src/rust/`
4. Move web server files from `web/` → `src/web/`

### Phase 2: Web Assets
1. Move `web/static/` → `web/static/`
2. Move `web/styles/` → `web/static/css/`
3. Clean up web directory to contain only frontend

### Phase 3: Scripts Organization
1. Create `scripts/build/`, `scripts/deploy/`, `scripts/test/`
2. Categorize existing scripts appropriately
3. Update script paths in documentation

### Phase 4: Configuration
1. Move `docker/` → `config/docker/`
2. Move `systemd/` → `config/systemd/`
3. Create `config/cmake/` for CMake modules

### Phase 5: Documentation
1. Reorganize `docs/` by category
2. Move relevant files to appropriate subdirectories

### Phase 6: Tests
1. Create `tests/` directory structure
2. Move test files from various locations

## Benefits

1. **Clear separation**: Source code, web assets, docs, scripts
2. **Logical grouping**: Related functionality grouped together
3. **Consistent naming**: Standardized directory names
4. **Scalability**: Easy to add new components
5. **Professional structure**: Follows common project conventions
6. **Better navigation**: Easier to find specific files

## Files to Update

After restructuring, update:
- `CMakeLists.txt` (source paths)
- `README.md` (structure documentation)
- Build scripts (new paths)
- Documentation (new structure)
- CI/CD configurations (if any)
