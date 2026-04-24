# Magic Leap 2 Depth Navigation

A native Android (NDK) application for the **Magic Leap 2** that uses the device's depth camera to detect passive IR marker spheres and track rigid-body surgical tools in real time. It is a C++ port and adaptation of the HoloLens 2 IR tracking system, rebuilt against the ML2 Depth Camera C API and OpenCV.

The app runs as a `NativeActivity` (no Java/Kotlin) and renders the depth camera streams as head-locked AR quads, with an in-headset ImGui control panel for runtime tuning and on-device tool capture.

![Demonstration](thesis-documentation/Demonstration.gif)

> The full write-up is available in [**ML2 Navigation Library Thesis — Yusuf Nissar**](<thesis-documentation/ML2 Navigation Library Thesis - Yusuf Nissar.pdf>), with an accompanying [**thesis presentation**](https://docs.google.com/presentation/d/1VHmRCzZGd_FDk7JqUE4PkSeI5OtqS3ucChiRGHYw-3g/edit?usp=sharing).

## Pipeline at a glance

Each frame flows through a three-stage pipeline:

1. **Acquire** — Pull the latest depth frame (raw intensity, calibrated depth in metres, ambient intensity, confidence) from `MLDepthCameraGetLatestDepthData`, together with the camera pose and intrinsics.
2. **Detect & Track** — `MarkerDetection` finds bright IR blobs and back-projects them to 3D camera space. `ToolTracker` matches those points against registered tool templates via pairwise-distance graph search, computes a 6DOF pose with Kabsch + per-sphere Kalman filtering, and transforms the result into world space.
3. **Visualize & Tune** — Depth streams are drawn as textured AR quads with colored blob overlays; tracked tool poses appear in the ImGui panel and are logged to logcat. Every stage of the pipeline is tunable live from the headset.

For a full walkthrough of the code, data flow, and units, see [`thesis-documentation/depth_navigation_structure.md`](thesis-documentation/depth_navigation_structure.md).

## Repository layout

```
magic-leap-2-navigation/
├── CppPlugin/
│   ├── depth_navigation/          # Main native Android app (Gradle + CMake)
│   │   ├── app/src/main/cpp/      # All C++ source — see the structure doc
│   │   ├── OpenCV-android-sdk/    # Vendored OpenCV (user-provided)
│   │   └── scripts/               # Build helpers (compile_commands, keystore)
│   └── app_framework/             # ML2 C API sample app framework (vendored)
├── ML2-CAPI-docs/                 # Local copy of ML2 C API Doxygen output
└── thesis-documentation/
    ├── SETUP.md                   # Toolchain versions, env vars, build & run
    ├── depth_navigation_structure.md  # High-level code architecture
    └── CLEANUP_PLAN.md
```

The key C++ modules inside `CppPlugin/depth_navigation/app/src/main/cpp/` are:

| File | Role |
|------|------|
| `main.cpp` | `DepthCameraApp`, frame loop, visualization, ImGui panel, tool capture |
| `marker_detection.{h,cpp}` | IR blob detection, area filtering, 3D back-projection |
| `tool_definition.{h,cpp}` | `TrackedTool` / `ToolTracker` types, registration, distance maps |
| `tool_tracking.cpp` | Graph search, conflict resolution, Kabsch pose + smoothing |
| `tool_kalman_filter.h` | Per-sphere constant-velocity Kalman filter |
| `depth_material.h` / `confidence_material.h` / `marker_overlay_material.h` | GLSL materials for the AR visualization |

## Getting started

Full setup (required tool versions, SDK paths, environment variables, OpenCV placement, and build commands) lives in [`thesis-documentation/SETUP.md`](thesis-documentation/SETUP.md). The short version:

1. Enable Developer Mode on an ML2 running OS 12.1 and connect it via USB-C.
2. Install Unity 2022.3, ML Hub 3, MLSDK 1.12, Android NDK 26.2.11394342, CMake 3.22.1, and JDK 11.
3. Place the OpenCV Android SDK at `CppPlugin/depth_navigation/OpenCV-android-sdk/`.
4. Create `CppPlugin/depth_navigation/local.properties` with your `sdk.dir`, `ndk.dir`, and `cmake.dir`.
5. Build, install, and launch:

   ```sh
   cd CppPlugin/depth_navigation
   ./gradlew assembleMl2Debug && \
   adb install -r app/build/outputs/apk/ml2/debug/com.magicleap.capi.sample.depth_camera-debug.apk && \
   adb shell am start -S -n com.magicleap.capi.sample.depth_camera/android.app.NativeActivity
   ```

   Grant the **Depth Camera** and **Camera** permissions on first launch.

## Debugging

The app emits categorized logs to `logcat`. Useful filters:

```sh
# All app logs
adb logcat -s MarkerDetection:I MarkerDepth:I MarkerPair:I ToolDef:I ToolTrack:V \
           com.magicleap.capi.sample.depth_camera:I

# Just the tracker
adb logcat -s ToolTrack
```

See [`SETUP.md`](thesis-documentation/SETUP.md#debugging-with-logcat) for the complete tag reference.

## Requirements

- Magic Leap 2 headset, Magic Leap OS 12.1, Developer Mode enabled
- MLSDK 1.12 and the ML2 C API (`ml_depth_camera`, `ml_head_tracking`, `ml_perception`)
- OpenCV Android SDK (vendored, user-provided)
- Android SDK 33 / NDK 26.2.11394342, CMake 3.22.1, JDK 11, Gradle (AGP 7.4.2), `x86_64` ABI
- Permissions: `com.magicleap.permission.DEPTH_CAMERA`, `android.permission.CAMERA`

## Further reading

- [`thesis-documentation/SETUP.md`](thesis-documentation/SETUP.md) — toolchain, environment, build/run, logcat
- [`thesis-documentation/depth_navigation_structure.md`](thesis-documentation/depth_navigation_structure.md) — architecture, data flow, per-file breakdown, unit conventions
- [`ML2 Navigation Library Thesis - Yusuf Nissar.pdf`](<thesis-documentation/ML2 Navigation Library Thesis - Yusuf Nissar.pdf>) — full thesis write-up
- [Thesis presentation (Google Slides)](https://docs.google.com/presentation/d/1VHmRCzZGd_FDk7JqUE4PkSeI5OtqS3ucChiRGHYw-3g/edit?usp=sharing)
- [`ML2-CAPI-docs/`](ML2-CAPI-docs/) — local ML2 C API reference

## Contact

Questions, or want additional documentation shared with you? Reach out to **Yusuf Nissar** at `yusufnissar8 [at] gmail [dot] com`.
