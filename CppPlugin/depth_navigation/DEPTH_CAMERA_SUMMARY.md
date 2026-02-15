# Magic Leap 2 Depth Camera - Technical Summary

## Overview

The depth camera sample is a C++ native application that demonstrates how to access, configure, and visualize the Magic Leap 2's depth camera capabilities. It streams live depth data from the device's depth sensors and renders multiple visual representations in AR space.

## Core Functionality

### 1. Depth Camera Initialization
- Connects to the ML2 depth camera hardware using the Magic Leap C API (`ml_depth_camera.h`)
- Requests `DEPTH_CAMERA` permission at runtime
- Supports both **LongRange** and **ShortRange** camera modes

### 2. Data Streams

The application captures and visualizes four distinct data types:

| Stream Type | Purpose | Value Range | Unit |
|------------|---------|-------------|------|
| **DepthImage** | Processed depth measurements | 0.0 - 5.0 (default) | meters |
| **Confidence** | Measurement confidence scores | 0.0 - 100.0 | percentage |
| **AmbientRawDepthImage** | Raw depth with ambient light | 5.0 - 2000.0 | raw units |
| **RawDepthImage** | Unprocessed depth data | 5.0 - 2000.0 | raw units |

### 3. Camera Modes

#### LongRange Mode
- Optimized for distant object detection
- Configurable frame rates: 1Hz, 5Hz, 25Hz, 30Hz, 50Hz, 60Hz
- Adjustable exposure settings based on selected frame rate

#### ShortRange Mode
- Optimized for close-proximity depth sensing
- Same frame rate options as LongRange

## Architecture

### Main Components

#### DepthCameraApp (main.cpp:108-795)
The primary application class that:
- Manages depth camera lifecycle (connect/disconnect)
- Handles permissions and restricted resource setup
- Coordinates preview rendering and GUI updates
- Processes depth frames asynchronously

#### Visualization System

**Preview Rendering:**
- Creates 4 preview quads arranged in a 2x2 grid
- Each quad displays one data stream type
- Quads are positioned 2.5 meters in front of the user
- Head-tracked to move with user's viewpoint

**Materials:**

1. **DepthMaterial** (depth_material.h)
   - Visualizes depth data using color mapping
   - GLSL shader normalizes depth values to 0-1 range
   - Maps normalized values to color gradient texture
   - Supports adjustable min/max depth thresholds

2. **ConfidenceMaterial** (confidence_material.h)
   - Combines depth and confidence data
   - Green = high confidence, Red = low confidence
   - Brightness modulated by depth (darker = farther)

### Data Flow

```
1. MLDepthCameraGetLatestDepthData()
   ↓
2. Extract frame buffers for each data type
   ↓
3. Upload float arrays to OpenGL textures
   ↓
4. Render quads with custom shaders
   ↓
5. MLDepthCameraReleaseDepthData()
```

## Key Features

### Real-time Configuration
Users can dynamically adjust:
- **Data streams**: Enable/disable individual data types via checkboxes
- **Camera mode**: Switch between LongRange/ShortRange
- **Frame rate**: Select from supported rates (1-60 Hz)
- **Exposure**: Fine-tune sensor exposure time (in microseconds)
- **Visualization range**: Adjust min/max depth thresholds for color mapping

### Frame Metadata Display
The GUI shows:
- Camera pose (position xyz, rotation xyzw)
- Frame number and timestamp
- Camera intrinsics (focal length, principal point, FOV, distortion coefficients)
- Min/max values in current frame
- Active frame type (LongRange/ShortRange)

### Async Settings Updates
- Camera settings updates run asynchronously using `std::async`
- Frame acquisition pauses during settings updates to prevent conflicts
- Prevents blocking the render loop

## Technical Details

### Texture Management
- All depth data stored as **GL_R32F** (single-channel 32-bit float) textures
- Texture dimensions: 544x480 (width x height) by default
- Automatically resizes if frame buffer dimensions change
- Blank depth texture used when depth stream is disabled

### Frame Synchronization
- Non-blocking depth data acquisition (timeout = 0ms)
- Tracks frame numbers to detect dropped frames
- Timestamps converted from MLTime to system time (HH:MM:SS format)

### Head Tracking Integration
- Uses `MLHeadTracking` API to update preview position
- Preview quads remain fixed relative to user's head coordinate frame
- GUI placed with offset to avoid obscuring depth visualizations

### Coordinate System
- Preview arranged in grid: 2x2 layout at z = -2.5m
- Scale factor: 1.25 / sqrt(num_previews)
- GUI offset: (+0.25m, 0, -2m) from head pose

## Shader Implementation

### Depth Visualization Shader
```glsl
// Normalizes depth to 0-1 range
normalized_depth = (depth - MinDepth) / (MaxDepth - MinDepth)

// Samples color from gradient texture
color = texture(HelperTexture, normalized_depth)

// Black for out-of-range values
if (depth < MinDepth || depth > MaxDepth) color = black
```

### Confidence Visualization Shader
```glsl
// Clamps confidence to 0-0.5 range, then normalizes
normalized_conf = clamp(abs(confidence), 0.0, 0.5) / 0.5

// Green-Red gradient (green = high confidence)
color = vec3(conf, 1.0 - conf, 0.0)

// Modulate brightness by depth
color *= (1.0 - normalized_depth / 2.0)
```

## Performance Considerations

- **Frame acquisition**: Non-blocking to prevent render stalls
- **Buffer management**: Always release depth data after processing to prevent buffer overflow
- **Async operations**: Settings updates run on separate thread
- **Texture uploads**: Direct GPU upload from depth buffers (no CPU copy)
- **Preview invalidation**: Rebuilds preview only when texture dimensions change

## Error Handling

- Validates data buffer sizes and byte alignment (expects float type)
- Checks for valid depth camera context before operations
- Handles timeout results gracefully (MLResult_Timeout)
- Detects and logs lost frames when frame numbers skip
- Warns about unsupported flags/indices with fallback defaults

## Use Cases

This sample demonstrates patterns for:
- AR-based depth sensing applications
- Obstacle detection and avoidance
- 3D environment scanning
- Real-time depth map visualization
- Camera intrinsics calibration validation
- Multi-stream sensor data fusion
