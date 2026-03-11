# Tool Tracking — Implementation Roadmap

This document describes the remaining features needed to bring the ML2 tool tracking
implementation to parity with the HoloLens 2 reference (`HoloLens2-IRTracking-main/`).
Features are ordered from highest to lowest impact.

---

## What Is Already Done

| Feature | Files |
|---------|-------|
| IR blob detection (intensity threshold → connected components → 3D position) | `marker_detection.h/.cpp` |
| Pairwise-distance graph search to match detected markers to a tool template | `tool_tracking.cpp` — `TrackTool()` |
| Greedy multi-tool conflict resolution | `tool_tracking.cpp` — `UnionSegmentation()` |
| Kabsch SVD rigid-body alignment | `tool_tracking.cpp` — `MatchPointsKabsch()` |
| Low-pass position filter + quaternion slerp on final pose | `tool_tracking.cpp` — `MatchPointsKabsch()` |
| Per-tool `min_visible_spheres` / partial occlusion | `tool_definition.h`, `tool_tracking.cpp` |
| Runtime add/remove tools | `tool_definition.cpp` |
| ImGui live display of tool pose | `main.cpp` |

Output format (same as HL2): 8×1 `cv::Mat` — `[x, y, z (m), qx, qy, qz, qw, valid]`

---

## Feature 1 — Per-Sphere Kalman Filtering ⭐ Highest Priority

### Why
Without this, each detected sphere's 3D position jumps frame-to-frame due to sensor
noise. The current code runs Kabsch on raw noisy positions, then smooths the *result*.
Smoothing the *inputs* first gives a dramatically more stable 6DOF pose because
individual sphere jitter is suppressed before the rigid-body fit.

The HL2 reference uses one `IRToolKalmanFilter` per sphere, applied inside
`MatchPointsKabsch` before accumulating sphere positions into matrix `q`.

### How — Step by Step

**1. Copy `IRKalmanFilter.h` into the ML2 source directory**

The file (`HoloLens2-IRTracking-main/IRKalmanFilter.h`) is pure OpenCV and has zero
Windows/WinRT dependencies — it compiles on Android as-is. Copy it to:
```
CppPlugin/depth_navigation/app/src/main/cpp/tool_kalman_filter.h
```
Rename the class to `ToolKalmanFilter` inside namespace `ml::tool_tracking` so it fits
the existing namespace structure.

The filter internals to keep:
- State: 6D `[x, y, z, vx, vy, vz]` (constant-velocity model)
- Measurement: 3D `[x, y, z]` (position only observed)
- Default noise: `positionNoise=1e-4`, `velocityNoise=3.0`, `measurementNoise=1.0`
- Lazy initialisation: first call to `FilterData` sets initial state to the observed value

**2. Add filter vector to `TrackedTool` in `tool_definition.h`**

```cpp
// In struct TrackedTool, add:
#include "tool_kalman_filter.h"

std::vector<ToolKalmanFilter> sphere_kalman_filters;
```

**3. Initialise one filter per sphere in `ToolTracker::AddTool` (`tool_definition.cpp`)**

```cpp
// After constructing the tool, before push_back:
for (int i = 0; i < tool.num_spheres; ++i)
    tool.sphere_kalman_filters.emplace_back();
```

**4. Apply the filter in `MatchPointsKabsch` (`tool_tracking.cpp`)**

Find the loop that fills matrix `q` with detected sphere positions (currently ~line 200).
After computing `sq` (the detected sphere in mm), filter it before use:

```cpp
// Before:
cv::Vec3f sq = frame_spheres_mm.at<cv::Vec3f>(sphere_ids[i], 0);

// After:
cv::Vec3f sq_raw = frame_spheres_mm.at<cv::Vec3f>(sphere_ids[i], 0);
cv::Vec3f sq = tool.sphere_kalman_filters[tnid].FilterData(sq_raw);
```

Note: `tnid` already tracks the correct tool node index (skipping occluded nodes), so
indexing into `sphere_kalman_filters` by `tnid` is correct.

**5. Expose noise parameters on `TrackedTool` (optional but recommended)**

```cpp
// In TrackedTool:
float kalman_measurement_noise{1.f};
float kalman_position_noise{1e-4f};
float kalman_velocity_noise{3.f};
```

Pass these to the `ToolKalmanFilter` constructor in `AddTool`.

### Verification
With Kalman enabled, individual sphere overlays in the ImGui debug view should stop
flickering between frames. The final pose `[x,y,z]` should be noticeably smoother than
the slerp-only version, especially at arm's length (where depth noise is highest).

---

## Feature 2 — World-Space Pose Output

### Why
Currently `MatchPointsKabsch` outputs the tool pose in **camera space** — relative to
the ML2 depth camera at the moment of capture. This means the pose changes whenever the
user moves their head, even if the tool is stationary. World space is required for any
stable AR overlay, multi-frame tracking, or downstream navigation logic.

### How — Step by Step

**1. Understand the ML2 pose API**

The app already acquires a head pose snapshot in `main.cpp`:
```cpp
MLSnapshot *snapshot = nullptr;
MLPerceptionGetSnapshot(&snapshot);
```
And head static data is initialised in `OnStart()`:
```cpp
MLHeadTrackingGetStaticData(head_tracker, &head_static_data_);
```

To get the camera-to-world transform at the time of a depth frame, use:
```cpp
MLTransform head_transform;
MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform);
```
`MLTransform` contains `position` (MLVec3f) and `rotation` (MLQuaternionf).

**2. Convert `MLTransform` to a 4×4 `cv::Mat`**

Write a helper (e.g. in `tool_definition.h`):
```cpp
cv::Mat MLTransformToCvMat(const MLTransform& t);
```
Build a standard 4×4 homogeneous matrix:
```
R  | p
0  | 1
```
where `R` comes from converting the quaternion `t.rotation` to a rotation matrix, and
`p` is `t.position` in metres.

**3. Update `ProcessFrame` signature to accept the pose**

```cpp
// tool_definition.h:
void ProcessFrame(
    const std::vector<ml::marker_detection::DetectedMarker>& markers,
    const cv::Mat& camera_to_world_4x4);   // NEW — 4×4 CV_32F
```

**4. Apply the transform inside `MatchPointsKabsch`**

After computing the Kabsch translation `t` (in mm, camera space), promote the result to
world space:
```cpp
// Build 4×4 transform in camera space (mm)
cv::Mat T_cam = cv::Mat::eye(4, 4, CV_32F);
R.copyTo(T_cam(cv::Rect(0, 0, 3, 3)));
T_cam.at<float>(0, 3) = t.at<float>(0, 0);
T_cam.at<float>(1, 3) = t.at<float>(1, 0);
T_cam.at<float>(2, 3) = t.at<float>(2, 0);

// Convert mm → m on translation column
T_cam.at<float>(0, 3) /= 1000.f;
T_cam.at<float>(1, 3) /= 1000.f;
T_cam.at<float>(2, 3) /= 1000.f;

// World transform = camera_to_world * T_cam
cv::Mat T_world = camera_to_world_4x4 * T_cam;

// Extract position and rotation from T_world for the output vector
```

**5. Update call site in `main.cpp`**

```cpp
cv::Mat cam_to_world = MLTransformToCvMat(head_transform);
tool_tracker_.ProcessFrame(detected_markers_, cam_to_world);
```

### Verification
Place the tool on a table, move your head side to side — the reported `[x, y, z]` in the
ImGui display should remain stable. Without world-space transform it will shift with
every head movement.

---

## Feature 3 — Runtime Tolerance Tuning via ImGui

### Why
The graph-search tolerances (`tolerance_side_` and `tolerance_avg_`, default 4 mm each)
directly control how strictly detected marker distances must match the template. In
practice the right values depend on sensor noise at the operating distance. Without live
tuning you have to recompile to test each value.

### How — Step by Step

**1. `SetTolerances` is already on `ToolTracker`** — no C++ changes needed.

**2. Add member variables in `DepthCameraApp` (`main.cpp`)**

```cpp
float tool_tolerance_side_ = 4.f;
float tool_tolerance_avg_  = 4.f;
```

**3. Add ImGui sliders in `UpdateGui` next to the existing Tool Tracking section**

```cpp
ImGui::Text("Tool Tracking");
{
    if (ImGui::SliderFloat("Side tolerance (mm)", &tool_tolerance_side_, 0.5f, 20.f))
        tool_tracker_.SetTolerances(tool_tolerance_side_, tool_tolerance_avg_);
    if (ImGui::SliderFloat("Avg tolerance (mm)",  &tool_tolerance_avg_,  0.5f, 20.f))
        tool_tracker_.SetTolerances(tool_tolerance_side_, tool_tolerance_avg_);
    // ... existing pose display ...
}
```

### Verification
With a real tool in view, decrease `tolerance_side_` until the tracker stops reporting
the tool — that lower bound is your sensor noise floor. Set both values ≈2× that floor.

---

## Feature 4 — Multi-Radius Frame Maps

### Why
If you want to track two tools simultaneously where one has 5 mm spheres and another has
8 mm spheres, the sphere-radius correction in `detectMarkerPositions` shifts each
sphere's 3D centroid by `depth + radius`. Using the wrong radius for a tool means the
pairwise distances in the frame map don't match the template, causing false negatives.

The HL2 reference handles this by building one `ProcessedAHATFrame` map per unique
radius (`spheres_xyz_per_mm` keyed by `sphere_radius`).

### How — Step by Step

**1. Change `ProcessFrame` to build a per-radius frame map**

Instead of a single `cv::Mat3f frame_spheres_mm`, build a
`std::map<float, cv::Mat3f> frame_spheres_per_radius` by iterating over all unique
`sphere_radius_mm` values across registered tools.

For each unique radius:
- Re-run the depth → 3D conversion with that radius correction
  (`depth + radius_mm/1000.f` in `detectMarkerPositions`, or re-derive from the
  already-returned `position_camera` values using: `pos_corrected = pos_raw * (depth + r) / depth`)
- Build a separate distance map and `ordered_sides` list

**2. Pass the correct frame map to each `TrackTool` call**

```cpp
for (auto& tool : tools_) {
    auto& frame_map   = frame_maps[tool.sphere_radius_mm];
    auto& frame_sides = frame_sides_map[tool.sphere_radius_mm];
    auto& frame_sph   = frame_spheres_per_radius[tool.sphere_radius_mm];
    TrackTool(tool, frame_sph, frame_map, frame_sides, num_frame_spheres, raw_results[i]);
}
```

### Verification
Register two tools with different radii. Confirm both are tracked simultaneously, and
that swapping the radii causes one or both to fail.

---

## Feature 5 — Expose Unfiltered Sphere Positions

### Why
Useful for debugging jitter, measuring filter latency, and building higher-level
algorithms (e.g. velocity estimation). HL2 stores these in `unfiltered_sphere_positions`
on `IRTrackedTool`.

### How — Step by Step

**1. Add field to `TrackedTool` in `tool_definition.h`**

```cpp
std::vector<cv::Vec3f> unfiltered_sphere_positions_mm; // filled each tracked frame
```

**2. Populate it in `MatchPointsKabsch` before Kalman filtering**

```cpp
tool.unfiltered_sphere_positions_mm.clear();
// ... existing loop ...
cv::Vec3f sq_raw = frame_spheres_mm.at<cv::Vec3f>(sphere_ids[i], 0);
tool.unfiltered_sphere_positions_mm.push_back(sq_raw);
cv::Vec3f sq = tool.sphere_kalman_filters[tnid].FilterData(sq_raw); // Feature 1
```

**3. Add to ImGui display (optional)**

```cpp
if (ImGui::CollapsingHeader("Raw Spheres")) {
    for (size_t s = 0; s < tool.unfiltered_sphere_positions_mm.size(); ++s) {
        const auto& p = tool.unfiltered_sphere_positions_mm[s];
        ImGui::Text("  sphere %zu: (%.1f, %.1f, %.1f) mm", s, p[0], p[1], p[2]);
    }
}
```

---

## Feature 6 — Visible-Spectrum Stereo Camera Fusion (Advanced)

### Why
The HL2 buffers left-front and right-front grayscale camera frames (`EnvFrame`) alongside
depth frames, enabling visible-light marker detection as a fallback when the IR signal is
weak. On ML2 the equivalent is the world camera or RGB camera APIs.

### How — High Level Only (complex)

1. Open the ML2 world camera using `MLCameraConnect` with `MLCameraIdentifier_CV` (the
   computer-vision camera, which is global-shutter and well-calibrated).
2. On each RGB frame, run a visible-light blob detector (e.g. blob detection on bright
   retro-reflective spots under ambient illumination).
3. Buffer these detections alongside depth frames with matching timestamps.
4. In `UnionSegmentation`, fuse: if a tool candidate from depth is ambiguous, use the
   visible-camera detection to disambiguate sphere assignment.

This is the most complex item and requires understanding the ML2 camera synchronization
API. Consider it only after Features 1–4 are complete.

---

## Suggested Implementation Order

```
Feature 1 (Kalman)         — ~2 hours — biggest quality gain, self-contained
Feature 3 (Tolerance GUI)  — ~30 min  — free once Feature 1 is done, helps tune 1 & 2
Feature 2 (World space)    — ~3 hours — required for navigation use case
Feature 4 (Multi-radius)   — ~2 hours — needed for multi-tool scenes
Feature 5 (Debug export)   — ~1 hour  — useful alongside any of the above
Feature 6 (Stereo fusion)  — ~1 week  — advanced, defer until core is stable
```
