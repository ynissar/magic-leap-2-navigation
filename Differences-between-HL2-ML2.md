# Differences Between HoloLens 2 and Magic Leap 2 Implementation

This document outlines the key differences between the HoloLens 2 IR tracking implementation (HoloLens2-IRTracking) and what will be required for the Magic Leap 2 port.

---

## 1. Platform & Operating System

| Aspect | HoloLens 2 | Magic Leap 2 |
|--------|------------|--------------|
| **OS** | Windows 10/11 (UWP) | Android (AOSP-based) |
| **Architecture** | ARM64 | ARM64 |
| **Runtime** | WinRT / C++/WinRT | Android NDK / Native C++ |
| **App Model** | Universal Windows Platform (UWP) | Android APK |

### Implications
- All Windows-specific APIs (WinRT, COM interfaces, Windows Perception) must be replaced with ML2 equivalents
- Build outputs change from `.dll` + `.winmd` to `.so` (shared object) libraries
- Unity integration changes from WinRT component to Android native plugin

---

## 2. Build System & Toolchain

| Aspect | HoloLens 2 | Magic Leap 2 |
|--------|------------|--------------|
| **IDE** | Visual Studio 2022 | Android Studio / VS Code |
| **Build System** | MSBuild (.vcxproj) | Gradle + CMake |
| **Project Files** | `HL2IRToolTracking.vcxproj`, `PropertySheet.props` | `build.gradle`, `CMakeLists.txt` |
| **Output** | `.dll` + `.winmd` | `.so` (shared library) |
| **NDK** | Not applicable | Android NDK required |

### Key Changes Required
- Convert Visual Studio project to CMake-based Android NDK project
- Replace NuGet packages with Gradle/CMake dependencies
- Compile OpenCV for Android ARM64 (different from UWP ARM64 build)

---

## 3. Sensor Access APIs

### HoloLens 2: Research Mode API
```cpp
// Research Mode sensor access
IResearchModeSensorDevice* m_pSensorDevice;
IResearchModeSensor* m_depthSensor;
IResearchModeCameraSensor* m_pDepthCameraSensor;

// Sensor types
ResearchModeSensorType::DEPTH_AHAT  // Short-throw depth + IR
ResearchModeSensorType::DEPTH_LONG_THROW

// Frame acquisition
pDepthSensorFrame->GetBuffer(&pDepth, &outBufferCount);      // Depth data
pDepthFrame->GetAbDepthBuffer(&pAbImage, &outAbBufferCount); // Active Brightness (IR)
```

### Magic Leap 2: Depth Camera API
```cpp
// ML2 depth camera access
#include <ml_depth_camera.h>

MLHandle cam_context_;
MLDepthCameraSettings camera_settings_;
MLDepthCameraData depth_camera_data_;

// Stream types
MLDepthCameraStream_ShortRange
MLDepthCameraStream_LongRange

// Frame buffers
MLDepthCameraFlags_DepthImage           // Processed depth
MLDepthCameraFlags_Confidence           // Confidence map
MLDepthCameraFlags_RawDepthImage        // Raw depth sensor data
MLDepthCameraFlags_AmbientRawDepthImage // Ambient light depth
```

### Critical Difference: Active Brightness / IR Image
- **HoloLens 2**: Provides dedicated `GetAbDepthBuffer()` for Active Brightness (IR reflections)
- **Magic Leap 2**: Provides `RawDepthImage` and `AmbientRawDepthImage` but **no explicit IR/AB image**

**Impact**: The core IR marker detection algorithm relies on detecting bright IR reflections in the AB image. Need to investigate:
1. Whether ML2's `RawDepthImage` contains IR intensity information
2. Alternative approaches using depth-only detection
3. Potential use of ML2's world cameras for marker detection

---

## 4. Spatial Coordinate Systems

### HoloLens 2: Windows Perception APIs
```cpp
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Preview.h>

// Spatial locator from sensor rig
SpatialGraphInteropPreview::CreateLocatorForNode(guid);
m_locator.TryLocateAtTimestamp(ts, m_refFrame);

// Transform to world coordinates
SpatialLocation location = ...;
XMMATRIX depthToWorld = depthCameraPoseInv * SpatialLocationToDxMatrix(location);
```

### Magic Leap 2: Perception APIs
```cpp
#include <ml_perception.h>
#include <ml_head_tracking.h>

// Head tracking setup
MLHeadTrackingCreate(&head_tracker);
MLHeadTrackingGetStaticData(head_tracker, &head_static_data_);

// Get pose at frame time
MLSnapshot* snapshot;
MLPerceptionGetSnapshot(&snapshot);
MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform);
```

### Key Differences
| Feature | HoloLens 2 | Magic Leap 2 |
|---------|------------|--------------|
| **Coordinate Frame** | SpatialCoordinateSystem | MLCoordinateFrameUID |
| **Locator** | SpatialLocator per sensor node | MLSnapshot-based queries |
| **Math Library** | DirectXMath (XMMATRIX, XMFLOAT4X4) | GLM (glm::mat4, glm::vec3) |
| **Camera Extrinsics** | `GetCameraExtrinsicsMatrix()` | `camera_pose` in frame data |

---

## 5. Camera Intrinsics & Projection

### HoloLens 2
```cpp
// Image point to camera unit plane mapping
m_pDepthCameraSensor->MapImagePointToCameraUnitPlane(uv, xy);
m_pDepthCameraSensor->MapCameraSpaceToImagePoint(xy, uv);

// LUT generation for unprojection
float z = 1.0f;
float norm = sqrtf(xy[0]*xy[0] + xy[1]*xy[1] + z*z);
// ... normalized direction vectors
```

### Magic Leap 2
```cpp
// Intrinsics provided per-frame
MLDepthCameraIntrinsics intrinsics = data.frames[0].intrinsics;
// Contains: width, height, focal_length, principal_point, fov, distortion[5]

// Manual unprojection using pinhole model
float x_cam = (u - cx) / fx;
float y_cam = (v - cy) / fy;
// Apply distortion correction using k1, k2, p1, p2, k3
```

### Key Changes
- Implement custom `MapImagePointToCameraUnitPlane()` equivalent using ML2 intrinsics
- Handle lens distortion correction (Brown-Conrady model with 5 distortion coefficients)
- Pre-compute LUT using ML2's intrinsic parameters

---

## 6. Depth Data Format

| Aspect | HoloLens 2 AHAT | Magic Leap 2 |
|--------|-----------------|--------------|
| **Resolution** | 512 x 512 | 544 x 480 (typical) |
| **Depth Format** | UINT16 (mm) | float32 (meters) |
| **Range (Short)** | ~0.2m - 1.0m | Configurable |
| **Range (Long)** | ~1.0m - 3.5m | Configurable |
| **Frame Rate** | Up to 45 Hz | 1-60 Hz (configurable) |

### Conversion Required
```cpp
// HoloLens 2: Depth in millimeters as UINT16
const UINT16* pDepth;
float depth_meters = pDepth[i] / 1000.0f;

// Magic Leap 2: Depth already in meters as float
float* depth_data = reinterpret_cast<float*>(depth_buffer->data);
float depth_meters = depth_data[i];
```

---

## 7. Permissions & Consent

### HoloLens 2
- Enable Research Mode on device (one-time setup in Device Portal)
- Declare capability in app manifest
- Runtime consent via `RequestCamAccessAsync()`

### Magic Leap 2
- Declare permission in `AndroidManifest.xml`:
  ```xml
  <uses-permission android:name="com.magicleap.permission.DEPTH_CAMERA" />
  ```
- Request permission at runtime using Android permission APIs
- No special "research mode" needed

---

## 8. Unity Integration

### HoloLens 2: WinRT Component
```
Unity Project/
└── Assets/
    └── Plugins/
        └── WSA/
            └── ARM64/
                ├── HL2IRToolTracking.dll
                └── HL2IRToolTracking.winmd
```
- C# scripts use WinRT interop
- Coordinate system passed via `SpatialCoordinateSystem` wrapper

### Magic Leap 2: Android Native Plugin
```
Unity Project/
└── Assets/
    └── Plugins/
        └── Android/
            └── libs/
                └── arm64-v8a/
                    └── libIRToolTracking.so
```
- C# scripts use P/Invoke or Unity Native Plugin interface
- Coordinate transforms handled differently (Unity world space alignment)

---

## 9. Threading Model

### HoloLens 2
```cpp
// Blocking sensor loop in dedicated thread
m_pDepthUpdateThread = new std::thread(DepthSensorLoop, this);

// Blocking frame acquisition
m_depthSensor->GetNextBuffer(&pDepthSensorFrame);
```

### Magic Leap 2
```cpp
// Non-blocking frame acquisition (timeout = 0)
MLDepthCameraGetLatestDepthData(cam_context_, 0, &depth_camera_data_);

// Async settings updates
std::future<MLResult> update_future = std::async(std::launch::async, 
    MLDepthCameraUpdateSettings, cam_context_, &camera_settings_);
```

---

## 10. Key Algorithm Components to Port

### Components That Can Be Reused (with modifications)
1. **IRToolTrack.cpp** - Core tracking algorithm (marker detection, pose estimation)
2. **IRKalmanFilter.h** - 3D position filtering
3. **IRStructs.h** - Data structures (minor modifications needed)
4. **Kabsch algorithm** - Point cloud registration for 6DOF pose

### Components Requiring Complete Rewrite
1. **HL2IRToolTracking.cpp** → **ML2IRToolTracking.cpp**
   - Replace Research Mode API with MLDepthCamera API
   - Replace Windows Perception with ML Perception
   - Replace DirectXMath with GLM
   
2. **Unity Bindings**
   - Convert from WinRT to Android native plugin interface
   - Adapt coordinate system handling

### Components Requiring Investigation
1. **IR Marker Detection** - Need alternative if ML2 lacks IR intensity image
   - Option A: Depth discontinuity detection for retro-reflective markers
   - Option B: Use ML2 world cameras with external IR illumination
   - Option C: Modify markers to use depth-detectable geometry

---

## 11. OpenCV Compilation

| Aspect | HoloLens 2 | Magic Leap 2 |
|--------|------------|--------------|
| **Platform** | UWP ARM64 | Android ARM64 |
| **Toolchain** | MSVC with ARM64 cross-compile | Android NDK (Clang) |
| **Runtime** | `/MD` (dynamic CRT) | libc++ |
| **Build** | Static library (opencv_world480.lib) | Static or shared (.a or .so) |

---

## 12. Summary of Major Porting Tasks

1. **Platform Adaptation**
   - [ ] Create CMake-based Android NDK project structure
   - [ ] Compile OpenCV 4.8.0 for Android ARM64
   - [ ] Set up Gradle build with native library integration

2. **API Replacement**
   - [ ] Replace Research Mode API with MLDepthCamera API
   - [ ] Replace Windows Perception with ML Perception/Head Tracking
   - [ ] Replace DirectXMath with GLM

3. **Sensor Integration**
   - [ ] Implement depth frame acquisition using ML2 API
   - [ ] Implement camera intrinsics unprojection
   - [ ] Handle coordinate frame transformations

4. **IR Detection Alternative**
   - [ ] Investigate ML2 raw depth data for IR information
   - [ ] Develop alternative marker detection if needed
   - [ ] Test with retro-reflective markers

5. **Unity Integration**
   - [ ] Create Android native plugin interface
   - [ ] Implement JNI bridge or C API for Unity
   - [ ] Adapt C# binding scripts

6. **Testing & Validation**
   - [ ] Verify depth data quality and range
   - [ ] Validate 3D point unprojection accuracy
   - [ ] Test tracking accuracy against HL2 baseline

---

## 13. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| No IR/AB image on ML2 | **High** | Investigate raw depth data; consider depth-based marker detection |
| Different depth characteristics | Medium | Tune detection thresholds; validate with test markers |
| Coordinate system differences | Medium | Careful transform chain implementation; thorough testing |
| OpenCV compatibility | Low | Well-documented Android NDK build process |
| Unity integration complexity | Medium | Follow ML2 Unity SDK patterns |

---

## References

- [Magic Leap 2 Depth Camera API](https://developer-docs.magicleap.cloud/docs/api-ref/api/Modules/group___camera/)
- [Magic Leap 2 Perception API](https://developer-docs.magicleap.cloud/docs/api-ref/api/Modules/group___perception/)
- [HoloLens 2 Research Mode](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/advanced-concepts/research-mode)
- Original HoloLens2-IRTracking: STTAR Framework
