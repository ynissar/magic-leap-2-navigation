# Implementation Plan: Marker Detection for Magic Leap 2

## Context

The goal is to implement IR marker detection for Magic Leap 2 (ML2) based on the HoloLens2 IR tracking code. The HoloLens2 code (`HoloLens2-IRTracking-main/IRToolTrack.cpp`) uses the AHAT (Articulated Hand Tracking) depth camera to detect bright IR markers by:

1. Thresholding the intensity image to find bright spots
2. Using connected components analysis to cluster pixels
3. Converting pixel coordinates to 3D camera space using depth data and camera intrinsics
4. Building distance maps between detected markers for tool tracking

We need to adapt this approach to Magic Leap 2's depth camera API, which has different data structures and lacks the `MapImagePointToCameraUnitPlane()` hardware API that HoloLens2 provides.

## Key Differences Between HoloLens2 and ML2

| Aspect            | HoloLens2                                         | Magic Leap 2                                  |
| ----------------- | ------------------------------------------------- | --------------------------------------------- |
| Intensity Data    | Separate AHAT intensity image (cvAbImage, 16-bit) | Part of raw_depth_image buffer (float)        |
| Depth Data        | UINT16 depth buffer                               | Float buffer in meters                        |
| Intrinsics        | Hardware API `MapImagePointToCameraUnitPlane()`   | Raw parameters in `MLDepthCameraIntrinsics`   |
| Camera Pose       | Separate query                                    | Per-frame in `MLDepthCameraFrame.camera_pose` |
| Coordinate System | DirectX (right-handed, row-major matrices)        | OpenGL-like (varies by framework)             |

---

## Implementation Steps

### 1. Create/Update `marker_detection.h` Header File

**File:** `CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.h`

```cpp
#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <ml_depth_camera.h>

// Marker detection configuration
struct MarkerDetectionConfig {
    float intensity_threshold_min = 1280.0f;  // Min raw ToF value for bright markers
    float intensity_threshold_max = 2000.0f;  // Max to avoid saturation
    int min_area_pixels = 10;                  // Minimum blob size
    int max_area_pixels = 180;                 // Maximum blob size
    float sphere_radius_mm = 6.0f;             // Physical marker radius
};

// Detected marker information
struct DetectedMarker {
    cv::Vec3f position_camera;  // 3D position in camera space [x, y, z] in mm
    cv::Point2f centroid_pixel; // 2D centroid in pixel coordinates
    int area_pixels;            // Blob area
    float intensity;            // Average intensity
};

class MarkerDetection {
public:
    // Static method for easy integration
    static std::vector<DetectedMarker> detectMarkerPositions(
        float* raw_depth_buffer,
        int width,
        int height,
        const MLDepthCameraIntrinsics& intrinsics,
        const MarkerDetectionConfig& config = MarkerDetectionConfig()
    );

private:
    // Helper: Convert pixel coordinates to normalized camera plane
    static void pixelToCameraPlane(
        float u, float v,
        const MLDepthCameraIntrinsics& intrinsics,
        float& x, float& y
    );

    // Helper: Perform connected component analysis
    static std::vector<DetectedMarker> findMarkerBlobs(
        const cv::Mat& binary_image,
        const cv::Mat& intensity_image,
        float* raw_depth_buffer,
        int width,
        int height,
        const MLDepthCameraIntrinsics& intrinsics,
        const MarkerDetectionConfig& config
    );
};
```

---

### 2. Implement `marker_detection.cpp`

**File:** `CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.cpp`

#### 2a. Main Detection Function

```cpp
std::vector<DetectedMarker> MarkerDetection::detectMarkerPositions(
    float* raw_depth_buffer,
    int width,
    int height,
    const MLDepthCameraIntrinsics& intrinsics,
    const MarkerDetectionConfig& config)
{
    // 1. Convert raw_depth_buffer to cv::Mat for processing
    cv::Mat raw_depth_mat(height, width, CV_32FC1, raw_depth_buffer);

    // 2. Apply thresholding to find bright markers
    //    raw_depth_image contains ToF sensor intensity values
    cv::Mat binary_mask;
    cv::inRange(raw_depth_mat, config.intensity_threshold_min,
                config.intensity_threshold_max, binary_mask);

    // 3. Convert to 8-bit for connectedComponentsWithStats
    cv::Mat binary_8bit;
    binary_mask.convertTo(binary_8bit, CV_8UC1);

    // 4. Find connected components and convert to 3D positions
    return findMarkerBlobs(binary_8bit, raw_depth_mat, raw_depth_buffer,
                          width, height, intrinsics, config);
}
```

#### 2b. Pixel-to-Camera-Plane Conversion

Replaces HoloLens2's `MapImagePointToCameraUnitPlane()`:

```cpp
void MarkerDetection::pixelToCameraPlane(
    float u, float v,
    const MLDepthCameraIntrinsics& intrinsics,
    float& x, float& y)
{
    // Basic pinhole camera model (no distortion correction for now)
    // x = (u - cx) / fx
    // y = (v - cy) / fy
    x = (u - intrinsics.principal_point.x) / intrinsics.focal_length.x;
    y = (v - intrinsics.principal_point.y) / intrinsics.focal_length.y;

    // NOTE: For high accuracy, should apply distortion correction:
    // - Use cv::undistortPoints() with distortion coefficients
    // - intrinsics.distortion contains [k1, k2, p1, p2, k3]
}
```

#### 2c. Connected Component Analysis

Adapted from `IRToolTrack.cpp` lines 658-692:

```cpp
std::vector<DetectedMarker> MarkerDetection::findMarkerBlobs(
    const cv::Mat& binary_image,
    const cv::Mat& intensity_image,
    float* raw_depth_buffer,
    int width, int height,
    const MLDepthCameraIntrinsics& intrinsics,
    const MarkerDetectionConfig& config)
{
    std::vector<DetectedMarker> markers;

    // Run connected components analysis
    cv::Mat labels, stats, centroids;
    int num_components = cv::connectedComponentsWithStats(
        binary_image, labels, stats, centroids, 8);

    // Process each component (skip background at index 0)
    for (int i = 1; i < num_components; i++) {
        int area = stats.at<int32_t>(i, cv::CC_STAT_AREA);

        // Filter by area size
        if (area < config.min_area_pixels || area > config.max_area_pixels) {
            continue;
        }

        // Get centroid in pixel coordinates
        double u = centroids.at<double>(i, 0);
        double v = centroids.at<double>(i, 1);

        // Get depth at centroid pixel (from processed depth_image, not raw)
        // NOTE: This requires access to the processed depth buffer
        // For now, using raw depth as placeholder
        float depth_mm = raw_depth_buffer[static_cast<int>(v) * width + static_cast<int>(u)];

        // Convert to camera plane coordinates
        float x, y;
        pixelToCameraPlane(u, v, intrinsics, x, y);

        // Compute 3D position accounting for sphere radius
        // depth_corrected = depth + sphere_radius (sphere center is behind surface)
        float depth_corrected = depth_mm + config.sphere_radius_mm;

        // Scale camera plane coordinates by depth to get 3D position
        // Normalize direction vector and scale by corrected depth
        cv::Vec3f direction(x, y, 1.0f);
        float norm = cv::norm(direction);
        cv::Vec3f position_3d = (direction / norm) * depth_corrected;

        // Create marker entry
        DetectedMarker marker;
        marker.position_camera = position_3d;  // [x, y, z] in mm
        marker.centroid_pixel = cv::Point2f(u, v);
        marker.area_pixels = area;
        marker.intensity = intensity_image.at<float>(static_cast<int>(v), static_cast<int>(u));

        markers.push_back(marker);
    }

    return markers;
}
```

---

### 3. Update `CMakeLists.txt`

**File:** `CppPlugin/depth_navigation/app/src/main/cpp/CMakeLists.txt`

Change line 31:

```cmake
# Before:
add_library(depth_camera SHARED main.cpp)

# After:
add_library(depth_camera SHARED
    main.cpp
    marker_detection.cpp
)
```

---

### 4. Integrate into `main.cpp`

**File:** `CppPlugin/depth_navigation/app/src/main/cpp/main.cpp`

#### 4a. Add Include

Add after line 24:

```cpp
#include "marker_detection.h"
```

#### 4b. Modify `processFrame()` Function

Update the `processFrame()` method (starting at line 720) to call marker detection at the end:

```cpp
void processFrame() {
    ALOGI("processFrame called with frame_count: %u", depth_camera_data_.frame_count);

    if (depth_camera_data_.frame_count >= 1) {
        for (size_t i = 0; i < depth_camera_data_.frame_count; i++) {
            ALOGI("=== Frame %zu ===", i);

            // ... existing logging code ...

            // ========== MARKER DETECTION INTEGRATION ==========
            // Process marker detection using raw depth image
            if (depth_camera_data_.frames[i].raw_depth_image != nullptr) {
                auto* raw_buffer = depth_camera_data_.frames[i].raw_depth_image;
                float* raw_data = reinterpret_cast<float*>(raw_buffer->data);

                if (raw_data != nullptr && raw_buffer->width > 0 && raw_buffer->height > 0) {
                    // Get intrinsics
                    const auto& intrinsics = depth_camera_data_.frames[i].intrinsics;

                    // Configure marker detection
                    MarkerDetectionConfig config;
                    config.intensity_threshold_min = 1280.0f;
                    config.intensity_threshold_max = 2000.0f;
                    config.min_area_pixels = 10;
                    config.max_area_pixels = 180;
                    config.sphere_radius_mm = 6.0f;

                    // Detect markers
                    auto markers = MarkerDetection::detectMarkerPositions(
                        raw_data,
                        raw_buffer->width,
                        raw_buffer->height,
                        intrinsics,
                        config
                    );

                    // Log detected markers
                    ALOGI("Detected %zu IR markers", markers.size());
                    for (size_t m = 0; m < markers.size(); m++) {
                        const auto& marker = markers[m];
                        ALOGI("  Marker %zu: pixel(%.1f, %.1f) 3D(%.1f, %.1f, %.1f)mm area=%d",
                              m,
                              marker.centroid_pixel.x, marker.centroid_pixel.y,
                              marker.position_camera[0], marker.position_camera[1], marker.position_camera[2],
                              marker.area_pixels);
                    }
                }
            }
            // ========== END MARKER DETECTION ==========
        }
    }
}
```

---

## Critical Implementation Notes

### Data Source Selection

**Decision needed:** Which ML2 buffer to use for marker detection?

- **`raw_depth_image`**: ToF sensor data with illuminator ON (contains both intensity + depth-like values)
- **`ambient_raw_depth_image`**: ToF sensor data with illuminator OFF (ambient light only)

**Recommendation:** Use `raw_depth_image` as it's closest to HoloLens2's AHAT intensity image

**Alternative:** Subtract ambient from raw for better SNR: `intensity = raw - ambient`

### Depth Data Issue

**Problem:** The implementation needs BOTH intensity (for detection) and depth (for 3D positioning).

- `raw_depth_image` contains intensity-like ToF values (not calibrated depth)
- `depth_image` contains calibrated depth in meters

**Solution:** Must access BOTH buffers in parallel:

```cpp
float* raw_intensity = reinterpret_cast<float*>(frames[i].raw_depth_image->data);
float* calibrated_depth = reinterpret_cast<float*>(frames[i].depth_image->data);
```

### Coordinate System Considerations

- ML2 camera pose is in world coordinates (`MLTransform`)
- Marker positions are initially in camera space
- For tool tracking (future work), need to transform to world space:

```cpp
// Apply camera_pose transform to convert camera→world
// This is done in HoloLens2 at IRToolTrack.cpp:418
```

### Distortion Correction

Current implementation skips distortion correction for simplicity. For production use, implement using OpenCV:

```cpp
std::vector<cv::Point2f> distorted_pts = {cv::Point2f(u, v)};
std::vector<cv::Point2f> undistorted_pts;
cv::Mat camera_matrix = (cv::Mat_<double>(3,3) <<
    intrinsics.focal_length.x, 0, intrinsics.principal_point.x,
    0, intrinsics.focal_length.y, intrinsics.principal_point.y,
    0, 0, 1);
cv::Mat distortion_coeffs = (cv::Mat_<double>(1,5) <<
    intrinsics.distortion[0], intrinsics.distortion[1],
    intrinsics.distortion[2], intrinsics.distortion[3],
    intrinsics.distortion[4]);
cv::undistortPoints(distorted_pts, undistorted_pts, camera_matrix, distortion_coeffs);
```

---

## Verification Steps

### 1. Build Verification

- Clean build the project
- Check that `marker_detection.cpp` is compiled
- Verify OpenCV linking succeeds

### 2. Runtime Testing

- Deploy to Magic Leap 2 device
- Place IR reflective markers in camera view
- Check logcat output for:
  - "Detected N IR markers" messages
  - Marker positions and pixel coordinates
  - No crashes or null pointer errors

### 3. Calibration/Tuning

- Adjust `intensity_threshold_min`/`max` based on actual ToF values
- Verify `sphere_radius_mm` matches physical markers
- Check if ambient subtraction improves detection
- Tune `min_area_pixels` and `max_area_pixels` for marker size

### 4. Validation

- Compare detected 3D positions to known physical positions
- Verify coordinate system handedness
- Check if distortion correction is needed (compare undistorted vs raw)

---

## Files to Modify

| File                                                               | Changes                                                |
| ------------------------------------------------------------------ | ------------------------------------------------------ |
| `CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.h`   | Create new header with class definition                |
| `CppPlugin/depth_navigation/app/src/main/cpp/marker_detection.cpp` | Implement detection functions                          |
| `CppPlugin/depth_navigation/app/src/main/cpp/main.cpp`             | Add include, integrate detection into `processFrame()` |
| `CppPlugin/depth_navigation/app/src/main/cpp/CMakeLists.txt`       | Add `marker_detection.cpp` to build                    |

---

## Future Enhancements

- **Distance Map Construction**: Implement the map/side construction logic from `IRToolTrack.cpp:557-587` for multi-marker tracking
- **Temporal Filtering**: Add Kalman filtering like HoloLens2 implementation
- **Tool Tracking**: Implement the full tool tracking pipeline with pattern matching
- **Performance**: Optimize using multi-threading or GPU acceleration
- **Ambient Subtraction**: Implement `raw - ambient` for better marker isolation
