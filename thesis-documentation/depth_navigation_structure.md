# Depth Navigation App — High-Level Structure

## Overview

`CppPlugin/depth_navigation/` is a native Android (NDK) application for Magic Leap 2 that detects IR markers using the device's depth camera and tracks rigid-body surgical tools in real time. It is a port of the HoloLens 2 IR tracking system (`HoloLens2-IRTracking-main/`) adapted to use the ML2 depth camera C API and OpenCV.

The app runs as a `NativeActivity` — there is no Java/Kotlin code. All logic is in C++, compiled via CMake, and linked against the ML2 perception SDK and a vendored OpenCV build. The application's entry point is `android_main()` in `main.cpp`, which instantiates a `DepthCameraApp` and enters the ML2 app framework's run loop.

Each iteration of the run loop follows a three-stage pipeline:

1. **Acquire** — The app connects to the ML2 depth camera and, on each update, pulls the latest frame containing four data buffers: calibrated depth (metres), raw intensity, ambient raw intensity, and confidence. It also retrieves the camera's world-space pose (position + quaternion) and intrinsic parameters (focal length, principal point, distortion coefficients).

2. **Detect & Track** — The raw intensity and calibrated depth buffers are passed to the marker detection module, which identifies bright IR blobs, filters them, and back-projects them to 3D positions in camera space. These detected markers are then forwarded to the tool tracking module, which matches them against registered tool templates using pairwise-distance graph search, computes a 6DOF pose via the Kabsch algorithm, and transforms the result into world space using the camera pose.

3. **Visualize & Tune** — The depth/confidence streams are rendered as head-locked textured quads in AR space. Detected and rejected blobs are drawn as colored point overlays on the raw intensity quad. Tracked tools are visualized as 3D axis gizmos positioned at their world-space poses. An ImGui control panel allows runtime tuning of every parameter in the pipeline — from camera exposure and detection thresholds to graph search tolerances, smoothing factors, and Kalman filter noise. A built-in tool capture workflow lets users define new tool geometries on-device by averaging marker positions across multiple frames.

---

## Data Flow

The following diagram traces the path of data through the application, from the hardware sensor to the final visual output. Understanding this flow is essential for navigating the codebase, as each stage maps to a specific source file.

```
ML2 Depth Camera Hardware
       │
       │  MLDepthCameraGetLatestDepthData()
       │  Returns: raw intensity, calibrated depth, ambient raw, confidence buffers
       │           + camera pose (MLTransform) + intrinsics (MLDepthCameraIntrinsics)
       ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  AcquireNewFrames()  [main.cpp]                                        │
│  Pulls the latest depth frame. Updates GPU textures for each data      │
│  stream (depth, confidence, ambient, raw) so the visualization quads   │
│  reflect the current frame. Calls processFrame() for detection/track.  │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  processFrame()  [main.cpp]                                            │
│  Orchestrates the detect→track pipeline for each frame. Populates the  │
│  MarkerDetectionConfig from current UI slider values, calls detection,  │
│  feeds results into tool capture (if active) and tool tracking.        │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
              ┌─────────────────┼──────────────────────┐
              ▼                 │                      │
┌──────────────────────┐        │                      │
│  MarkerDetection::   │        │                      │
│  detectMarkerPositions│       │                      │
│  [marker_detection.cpp]       │                      │
│                      │        │                      │
│  Input:              │        │                      │
│   • raw intensity    │        │                      │
│     (float buffer)   │        │                      │
│   • calibrated depth │        │                      │
│     (float buffer, m)│        │                      │
│   • ambient raw      │        │                      │
│     (optional)       │        │                      │
│   • intrinsics       │        │                      │
│                      │        │                      │
│  Processing:         │        │                      │
│   ambient subtract → │        │                      │
│   Gaussian blur →    │        │                      │
│   intensity threshold│        │                      │
│   → morphological    │        │                      │
│   closing →          │        │                      │
│   connected comps →  │        │                      │
│   depth lookup →     │        │                      │
│   area filter →      │        │                      │
│   undistort & back-  │        │                      │
│   project to 3D     │        │                      │
│                      │        │                      │
│  Output:             │        │                      │
│   DetectedMarker[]   │        │                      │
│   (position in m,    │        │                      │
│    pixel centroid,   │        │                      │
│    area, intensity,  │        │                      │
│    blob pixels)      │        │                      │
│   RejectedBlob[]     │        │                      │
│   (with rejection    │        │                      │
│    reason for debug) │        │                      │
└──────────┬───────────┘        │                      │
           │                    │                      │
           ▼                    ▼                      ▼
┌───────────────────────────────────────┐   ┌─────────────────────┐
│  ToolTracker::ProcessFrame()          │   │  Tool Capture       │
│  [tool_tracking.cpp]                  │   │  (if active)        │
│                                       │   │  [main.cpp]         │
│  Input:                               │   │                     │
│   • DetectedMarker[] (metres)         │   │  Accumulates marker │
│   • camera-to-world 4×4 matrix        │   │  positions across N │
│                                       │   │  frames, reorders   │
│  Processing:                          │   │  by nearest-neighbor │
│   m→mm conversion →                   │   │  correspondence,    │
│   build pairwise distance map →       │   │  Kabsch-aligns all  │
│   graph search per tool (TrackTool) → │   │  frames to a mean   │
│   conflict resolution                 │   │  reference, then    │
│     (UnionSegmentation) →             │   │  registers the tool │
│   Kabsch pose estimation              │   │  with ToolTracker.  │
│     (MatchPointsKabsch) →             │   └─────────────────────┘
│   Kalman filtering (per-sphere) →     │
│   low-pass smoothing (pos + rot) →    │
│   camera→world transform             │
│                                       │
│  Output (per registered tool):        │
│   8×1 cv::Mat                         │
│   [x, y, z (m), qx, qy, qz, qw,     │
│    valid (0 or 1)]                    │
│   in world space                      │
└───────────────────┬───────────────────┘
                    │
      ┌─────────────┼──────────────────┐
      ▼             ▼                  ▼
┌────────────┐ ┌────────────┐  ┌──────────────┐
│ Update     │ │ Update     │  │ Update       │
│ Marker     │ │ Rejected   │  │ Tool         │
│ Overlay    │ │ Overlay    │  │ Visuals      │
│ [main.cpp] │ │ [main.cpp] │  │ [main.cpp]   │
│            │ │            │  │              │
│ Colored    │ │ Green      │  │ 3D axis      │
│ points on  │ │ points on  │  │ gizmos at    │
│ raw depth  │ │ raw depth  │  │ tool world   │
│ quad       │ │ quad       │  │ poses        │
└────────────┘ └────────────┘  └──────────────┘
```

---

## Directory Layout

```
depth_navigation/
├── app/
│   └── src/main/
│       ├── AndroidManifest.xml        # Permissions (DEPTH_CAMERA, CAMERA), NativeActivity entry point
│       ├── cpp/                        # All C++ source (see below)
│       │   └── CMakeLists.txt          # Build config — links OpenCV, ML SDK, app_framework
│       └── res/                        # Android resources (app icon, strings)
├── build.gradle                       # Android Gradle config (NDK 26.2, SDK 33, x86_64 ABI for ML2)
├── gradle.properties / settings.gradle
├── scripts/
│   └── generate_compile_commands.sh   # Helper for IDE tooling (clangd)
├── OpenCV-android-sdk/                # Vendored OpenCV (used by marker detection & tracking)
└── README.md / DEPTH_CAMERA_SUMMARY.md
```

---

## C++ Source Files (`app/src/main/cpp/`)

### `main.cpp` — Application entry point, frame loop, UI, and visualization

This is the largest file in the project and serves as the top-level orchestrator. It defines the `DepthCameraApp` class (a subclass of `ml::app_framework::Application`) which owns the depth camera connection, the marker detector configuration, the `ToolTracker` instance, all visualization state, and the ImGui control panel. The `android_main()` function at the bottom of the file instantiates this class and enters the run loop.

**Primary purposes:**
- Manage the ML2 depth camera lifecycle (connect, configure, acquire frames, release)
- Drive the per-frame detect→track pipeline by calling into `MarkerDetection` and `ToolTracker`
- Render depth camera data streams as AR-space quads and overlay detection results
- Expose an in-headset ImGui panel for runtime parameter tuning and tool capture

**Key functions:**

- **`OnStart()` / `OnResume()` / `OnPause()` / `OnStop()`** — Android lifecycle callbacks. `OnResume` connects to the depth camera and sets up the preview. `OnPause` disconnects the camera.

- **`OnUpdate(float)`** — Called every frame by the app framework. Moves the preview to follow the user's head, updates the GUI, and calls `AcquireNewFrames()` to pull and process the latest depth data.

- **`AcquireNewFrames()`** — Non-blocking call to `MLDepthCameraGetLatestDepthData`. On success, updates frame metadata, pushes each data stream's buffer into the corresponding GPU texture, then calls `processFrame()`.

- **`processFrame()`** — The core orchestration function. Constructs a `MarkerDetectionConfig` from the current UI slider values, calls `MarkerDetection::detectMarkerPositions()`, feeds the results into the tool capture state machine (if active), converts the camera pose to a 4×4 matrix via `MLTransformToCvMat()`, and calls `ToolTracker::ProcessFrame()`. Also triggers `UpdateMarkerOverlay()`, `UpdateRejectedOverlay()`, and `UpdateToolVisuals()`.

- **`UpdateGui()`** — Draws the ImGui control panel with sections for: basic frame info (camera pose, frame number), camera settings (stream mode, frame rate, exposure), marker overlay controls, tool tracking status (per-tool pose display), tool capture workflow (start/progress/register/discard), and tuning panels (marker detection thresholds, graph search tolerances, pose smoothing alpha, Kalman filter noise).

- **`MLTransformToCvMat(const MLTransform&)`** — Converts the ML2 camera pose (position in metres + unit quaternion) into a 4×4 row-major homogeneous matrix (`CV_32F`) for use as the camera-to-world transform in the tracking pipeline.

- **`SetupPreview()` / `DestroyPreview()`** — Create and tear down the four head-locked textured quads (one per data stream) with their legend bars, custom shader materials, and overlay attachment points.

- **`UpdateMarkerOverlay()` / `UpdateRejectedOverlay()`** — Convert detected marker or rejected blob pixel coordinates into point meshes and render them as colored overlays on the raw depth image quad.

- **`UpdateToolVisuals()`** — For each registered tool, creates (on first encounter) or updates a 3D axis gizmo node at the tool's world-space pose. Hides the gizmo when the tool is not currently tracked.

- **`FinalizeCapture()`** — Concludes the tool capture workflow. Performs iterative Kabsch-based alignment of all accumulated frames to a mean reference (3 iterations), computes a quality metric (max per-sphere standard deviation), subtracts the centroid, converts from metres to mm, and stores the result for user review before registration.

- **`AlignFrameToReference()`** — Helper for tool capture. Applies the Kabsch algorithm (SVD of cross-covariance) to rigidly align one frame's sphere positions to the current reference positions.

### `marker_detection.h / .cpp` — IR blob detection and 3D back-projection

This module is responsible for the first stage of the pipeline: finding bright IR marker blobs in the raw intensity image and computing their 3D positions in camera space. It operates entirely on a single frame's data with no temporal state (except for optional depth-scaling baseline tracking used for diagnostics).

**Primary purposes:**
- Identify bright regions in the raw intensity image that correspond to IR-reflective marker spheres
- Filter candidates by depth validity and expected projected area
- Back-project accepted blobs from 2D pixel coordinates to 3D camera-space positions using the calibrated depth and camera intrinsics
- Provide rejected-blob data for debug visualization

**Key functions:**

- **`MarkerDetection::detectMarkerPositions()`** — The main entry point. Takes raw intensity, calibrated depth, and (optionally) ambient intensity buffers along with image dimensions and camera intrinsics. Wraps the raw float buffers into `cv::Mat` objects. If ambient subtraction is enabled, subtracts the ambient buffer from raw intensity and clamps negatives to zero. Applies Gaussian blur (configurable kernel size), then `cv::inRange` thresholding between configurable min/max intensity values to produce a binary mask. Applies morphological closing (configurable kernel size) to merge nearby bright pixels into coherent blobs. Calls `findMarkerBlobs()` to classify each blob. Optionally logs pairwise inter-marker distances (for calibration debugging). Returns `std::vector<DetectedMarker>`.

- **`MarkerDetection::findMarkerBlobs()`** — Runs `cv::connectedComponentsWithStats` on the binary mask. For each connected component (skipping background): collects all blob pixels, looks up the centroid's calibrated depth, rejects blobs with out-of-bounds centroids or invalid depth values, computes expected projected area via `expectedMarkerAreaPixels()` and rejects area mismatches, then calls `pixelToCameraPlane()` to undistort and back-project. Applies a sphere-radius depth correction (adds `sphere_radius_mm / 1000` to the depth). Constructs the final 3D position as `(direction / ||direction||) * depth_corrected`. Populates rejected blob records with the specific `RejectionReason` for debug display.

- **`MarkerDetection::pixelToCameraPlane()`** — Converts pixel coordinates `(u, v)` to normalized camera plane coordinates `(x, y)` by building the 3×3 camera matrix and 5-element distortion vector from the ML2 intrinsics, then calling `cv::undistortPoints()`. The output coordinates represent the ray direction from the camera center through that pixel (with `z = 1`).

- **`MarkerDetection::expectedMarkerAreaPixels()`** — Computes the expected projected area of a marker sphere at a given depth using the formula `A_px ≈ π·r²·fx·fy / d²`, where `r` is the sphere radius in mm, `d` is the depth in mm, and `fx`, `fy` are the focal lengths in pixels. Used by the area filter to reject blobs that are too large or too small for their depth.

**Key data structures:**

- **`MarkerDetectionConfig`** — Holds all tuneable parameters: intensity threshold min/max, sphere radius, ambient subtraction toggle, Gaussian blur and morphology kernel sizes, area filter ratios, and logging options. Populated from UI sliders in `processFrame()`.

- **`DetectedMarker`** — A successfully detected marker: 3D position in camera space (metres), pixel centroid, blob area in pixels, centroid intensity, and the list of all blob pixels (for overlay rendering).

- **`RejectedBlob`** — A blob that failed classification: includes the same 2D info plus the rejection reason (`OutOfBounds`, `InvalidDepth`, or `AreaMismatch`) and expected area bounds (for `AreaMismatch`).

### `tool_definition.h / .cpp` — Tool data structures, registration, and pairwise distance map construction

This module defines the data structures that represent registered tools and the `ToolTracker` class's public API for tool management. It also implements the pairwise distance map construction that underpins the graph search. The header (`tool_definition.h`) is the central type definition file — both `main.cpp` and `tool_tracking.cpp` include it.

**Primary purposes:**
- Define the `TrackedTool` struct that stores a tool's template geometry, distance map, Kalman filters, smoothing parameters, and current pose output
- Define the `ToolTracker` class with its public API for registering/removing tools, processing frames, retrieving transforms, and tuning parameters
- Build the pairwise distance map and sorted side list used by the graph search

**Key functions:**

- **`ToolTracker::AddTool()`** — Registers a new tool from its sphere positions (`cv::Mat3f` in mm), sphere radius, identifier string, and minimum visible sphere count. Calls `ConstructMap()` to build the pairwise distance matrix and sorted side list. Initialises one `ToolKalmanFilter` per sphere. Logs the full tool geometry to logcat under the `ToolDef` tag.

- **`ToolTracker::RemoveTool()` / `RemoveAllTools()`** — Deregisters tools and rebuilds the internal index map to keep indices consistent.

- **`ToolTracker::GetToolTransform()`** — Returns a clone of the named tool's current 8×1 transform vector `[x, y, z (m), qx, qy, qz, qw, valid]`. Returns zeros if the tool is unknown or no tools are registered.

- **`ToolTracker::ConstructMap()`** — Builds an upper-triangular Euclidean distance matrix (mm) from the sphere positions and a distance-sorted `std::vector<Side>` for efficient seed search during graph matching.

- **`ToolTracker::SetTolerances()` / `SetLowpassFactors()` / `ResetKalmanFilters()`** — Parameter tuning methods called from the UI. `SetTolerances` configures absolute per-side tolerance, mean-error tolerance, and relative tolerance fraction. `SetLowpassFactors` sets the position blend and rotation slerp factors. `ResetKalmanFilters` reinitialises all per-sphere Kalman filters with new noise parameters.

- **`ToolTracker::GetSecondsSinceTracked()`** — Returns the elapsed time since the tool was last successfully tracked, or -1 if never tracked.

**Key data structures:**

- **`Side`** — A directed edge between two sphere indices with its Euclidean distance. Used in both the tool template and the per-frame distance map.

- **`ToolResult`** — One candidate pose solution: the matched sphere IDs (indices into the current frame's detected markers), the summed side-length error in mm, and the list of occluded tool node indices. Sorted by fewest occlusions first, then lowest error.

- **`TrackedTool`** — A registered rigid-body tool. Stores the template sphere positions (`spheres_xyz_mm`, N×1 `cv::Mat3f` in mm), the upper-triangular pairwise distance matrix (`map`), sorted side list (`ordered_sides`), per-sphere Kalman filters, low-pass smoothing factors, and the current 8×1 transform output (`cur_transform`). Also tracks the timestamp of the last successful pose update.

### `tool_tracking.cpp` — Graph search, conflict resolution, and Kabsch pose estimation

This file contains the frame-by-frame tracking pipeline — the computational core of the system. It takes detected marker positions and matches them against registered tool templates using a pairwise-distance graph search, resolves conflicts when multiple tools compete for the same markers, and computes world-space 6DOF poses via the Kabsch algorithm with temporal smoothing.

**Primary purposes:**
- Match detected markers to registered tool templates using distance-based graph search
- Handle partial occlusion (fewer visible markers than the tool definition expects)
- Resolve conflicts when multiple tools share detected markers
- Compute rigid-body transforms (rotation + translation) from matched point correspondences
- Apply temporal smoothing (Kalman filtering + low-pass) for stable output

**Key functions:**

- **`ToolTracker::ProcessFrame()`** — Entry point called once per depth frame. Converts detected marker positions from metres to mm (the internal geometry convention). Builds a pairwise Euclidean distance map and sorted side list for the current frame's markers via `ConstructMap()`. Runs `TrackTool()` independently for each registered tool to generate candidate matches. Passes all candidates to `UnionSegmentation()` for conflict resolution and final pose computation.

- **`ToolTracker::TrackTool()`** — The graph search algorithm. Operates as a depth-first search seeded on frame sides whose length matches the first visible template side (accounting for possible leading occlusions). For each seed, expands by testing whether additional detected markers' pairwise distances to already-visited markers match the template within tolerance. The effective per-side tolerance is `max(tolerance_side_mm, tolerance_rel * expected_side_mm)`, providing depth-scale invariance for larger inter-sphere distances. Candidates are accepted when all expected spheres (minus allowed occlusions) are matched and the mean error is below `max(tolerance_avg_mm, tolerance_rel * mean_template_side_mm)`. Produces a list of ranked `ToolResult` candidates.

- **`ToolTracker::UnionSegmentation()`** — Greedy conflict resolution across all tools. Pools all candidates from all tools, sorts by quality (fewest occlusions first, then lowest error), and iterates: takes the best remaining candidate, computes its pose via `MatchPointsKabsch()`, then removes all other candidates that share any detected marker with the chosen result. This ensures each detected marker is assigned to at most one tool.

- **`ToolTracker::MatchPointsKabsch()`** — Computes the optimal rigid-body transform (rotation + translation) that maps the tool's template sphere positions to the detected positions. Per-sphere Kalman filters are applied to the detected positions before alignment to reduce measurement noise. The Kabsch algorithm uses SVD of the cross-covariance matrix between centroid-subtracted template and detected point sets to find the optimal rotation, with a determinant check to prevent reflections. The resulting camera-space transform (translation converted from mm to metres) is multiplied by the camera-to-world matrix to produce a world-space pose. The rotation matrix is converted to a quaternion via Shepperd's method. If the previous frame had a valid pose, low-pass smoothing is applied: linear interpolation for position (controlled by `lowpass_factor_position`) and spherical linear interpolation (slerp) for rotation (controlled by `lowpass_factor_rotation`). Outputs the 8×1 vector `[x, y, z (m), qx, qy, qz, qw, valid=1.0]`.

- **`ToolTracker::GetSecondsSinceTracked()`** — Returns elapsed time since the last successful pose update for a given tool, using `std::chrono::steady_clock`.

### `tool_kalman_filter.h` — Per-sphere Kalman filter

A header-only implementation of a 6-state constant-velocity Kalman filter, ported from the HoloLens 2 `IRKalmanFilter.h`. One filter instance is maintained per sphere per registered tool. It smooths individual sphere position measurements before they enter the Kabsch algorithm, reducing frame-to-frame jitter.

**Primary purposes:**
- Provide temporal smoothing of individual marker sphere positions
- Reduce measurement noise before pose estimation

The filter uses a constant-velocity motion model with state `[x, y, z, vx, vy, vz]` and measures only position `[x, y, z]`. It is lazily initialised on the first `FilterData()` call — the initial state is seeded from the first observation with zero velocity. All coordinates are in mm. The three noise parameters (measurement, position, velocity) are configurable from the UI via `ToolTracker::ResetKalmanFilters()`.

### `confidence_material.h` / `depth_material.h` — Custom GLSL shaders

OpenGL materials for visualizing depth camera data streams as color-mapped textured quads. `DepthMaterial` maps calibrated depth values to a color gradient texture. `ConfidenceMaterial` overlays confidence values on the depth visualization. Both support configurable min/max range via the ImGui UI.

### `marker_overlay_material.h` — Marker visualization shader

A simple solid-color GLSL material used to render detected marker blobs and rejected blobs as colored point overlays on the depth camera preview quads. Detected markers use a user-configurable color; rejected blobs are rendered in green.

---

## Build System

- **Gradle + CMake**: `build.gradle` configures the Android build (compileSdk 33, NDK 26.2, x86_64 ABI for ML2). CMake handles native compilation, linking OpenCV and the ML2 C API SDK (`ml_depth_camera`, `ml_head_tracking`, `ml_perception`).
- **Dependencies**: Vendored OpenCV (`OpenCV-android-sdk/`), Magic Leap app framework (fetched via CMake), ImGui (for the in-headset control panel).
- **Permissions**: `com.magicleap.permission.DEPTH_CAMERA` and `android.permission.CAMERA`.

---

## Unit Conventions

| Context                        | Unit        |
|-------------------------------|-------------|
| `DetectedMarker::position_camera` | metres      |
| Tool template (`spheres_xyz_mm`)  | millimetres |
| Graph search tolerances           | millimetres |
| Kalman filter internals           | millimetres |
| Final output transform `[xyz]`    | metres      |

`ProcessFrame()` converts detected marker positions from metres to mm before graph matching. `MatchPointsKabsch()` converts the result back to metres for the output transform.
