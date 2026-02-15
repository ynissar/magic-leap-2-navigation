# IR Marker Detection Implementation Summary

## Implementation Complete ✓

The IR marker detection system has been successfully implemented and compiled for Magic Leap 2.

## Files Created/Modified

### Created Files

1. **`CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.h`**
   - Declares `MarkerDetectionConfig` struct for configurable parameters
   - Declares `DetectedMarker` struct for detection results
   - Declares `MarkerDetection` class with static detection methods

2. **`CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.cpp`**
   - Implements intensity thresholding on raw depth data
   - Implements connected components analysis for blob detection
   - Implements pixel-to-3D conversion using camera intrinsics
   - Includes DEBUG_MODE sections for diagnostic logging

### Modified Files

3. **`CppPlugin/depth_navigation/app/src/main/cpp/main.cpp`**
   - Added `#include "marker_detection.h"`
   - Integrated marker detection in `processFrame()` function
   - Logs detected marker positions and properties

4. **`CppPlugin/depth_navigation/app/src/main/cpp/CMakeLists.txt`**
   - Added `marker_detection.cpp` to build sources

## Implementation Details

### Detection Pipeline

```
Raw Intensity Data
       ↓
Optional Ambient Subtraction (improves SNR)
       ↓
Intensity Thresholding (1280-2000 range)
       ↓
Connected Components Analysis (blob detection)
       ↓
Filter by Blob Size (10-180 pixels)
       ↓
Lookup Calibrated Depth at Centroid
       ↓
Convert to 3D Camera Coordinates
       ↓
Return Detected Markers
```

### Key Features

1. **Configurable Parameters** (via `MarkerDetectionConfig`):
   - `intensity_threshold_min/max`: Intensity range for bright markers
   - `min_area_pixels/max_area_pixels`: Valid marker blob size range
   - `sphere_radius_mm`: Physical marker size for correction (TODO)
   - `use_ambient_subtraction`: Enable/disable ambient light subtraction

2. **Detection Results** (via `DetectedMarker`):
   - `position_camera`: 3D position in meters (camera space)
   - `centroid_pixel`: 2D pixel coordinates of marker center
   - `area_pixels`: Blob area for validation
   - `intensity`: Brightness value for quality assessment

3. **Pixel-to-3D Conversion**:
   - Uses pinhole camera model with ML2 intrinsics
   - Normalizes direction vectors for correct depth scaling
   - Handles depth lookup from calibrated depth buffer

## Current Configuration

Default detection parameters in `main.cpp`:
```cpp
config.intensity_threshold_min = 1280.0f;
config.intensity_threshold_max = 2000.0f;
config.use_ambient_subtraction = false;
```

## Build Status

✅ **Debug Build**: Successful
⚠️ **Release Build**: Blocked by signing configuration (not code-related)

The C++ code compiles without errors on all architectures (x86_64, arm64-v8a).

## Next Steps for Testing

### 1. Enable Debug Logging

To see detailed detection diagnostics, uncomment in `marker_detection.cpp`:
```cpp
// Line 18: Change from
// #define DEBUG_MODE

// To
#define DEBUG_MODE
```

Or add to CMakeLists.txt:
```cmake
target_compile_definitions(depth_camera PRIVATE DEBUG_MODE)
```

### 2. Deploy to Device

```bash
cd CppPlugin/depth_navigation
./gradlew installMl2Debug
```

### 3. Monitor Logcat Output

```bash
adb logcat -s com.magicleap.capi.sample.depth_camera:I
```

Look for:
- `[DEBUG] Intensity range: min=X, max=Y` - to calibrate thresholds
- `[DEBUG] Bright pixels after threshold: N` - to verify detection
- `[DEBUG] Found N connected components` - to count candidate blobs
- `Detected N IR markers` - final detection count
- `Marker N: pixel(...) 3D(...)m` - marker positions

### 4. Calibration Steps

**Without Physical Markers (Initial Testing):**
1. Point camera at various scenes
2. Check DEBUG logs for intensity min/max values
3. Verify that bright objects trigger detections
4. Note typical bright pixel counts and blob areas

**With IR Reflective Markers:**
1. Place IR markers in view (3M Scotchlite or similar)
2. Adjust `intensity_threshold_min/max` based on marker brightness
3. Test `use_ambient_subtraction = true` vs `false`
4. Verify 3D positions are reasonable (not NaN, within expected range)
5. Check detection stability (consistent frame-to-frame)

### 5. Parameter Tuning

Based on testing results, adjust in `main.cpp`:

```cpp
// Example: If markers are brighter than expected
config.intensity_threshold_min = 1500.0f;  // Raise threshold
config.intensity_threshold_max = 2500.0f;

// Example: If background noise is high
config.use_ambient_subtraction = true;  // Enable ambient subtraction

// Example: If markers appear larger/smaller
config.min_area_pixels = 5;   // Adjust size range
config.max_area_pixels = 100;
```

## Open Questions (TODO in Code)

1. **Units**: Keep meters or convert to mm? (Currently using meters - ML2 native)
2. **Sphere Radius Correction**: Add or subtract? Correct magnitude? (Currently disabled)
3. **Distortion Correction**: Implement now or later? (Currently using simple pinhole model)
4. **Threshold Values**: Calibrate with actual markers? (Currently using estimated values)

## Future Enhancements

- [ ] Implement sphere radius correction for marker center offset
- [ ] Add distortion correction using `cv::undistortPoints`
- [ ] Implement temporal filtering (Kalman filter) for stable tracking
- [ ] Add distance map construction for multi-marker tool tracking
- [ ] Add pattern matching for tool identification
- [ ] Add camera-to-world coordinate transformation
- [ ] Performance optimization (frame skipping, ROI processing)

## References

- Plan file: `/Users/yusufnissar/.claude/plans/async-stirring-hummingbird.md`
- HoloLens2 reference: `HoloLens2-IRTracking-main/IRToolTrack.cpp`
- ML2 Depth Camera API: Magic Leap SDK documentation
