# ML2 Depth Navigation — Setup & Run Guide

Since the ML2 is very reliant on its dependencies, it is important to have the correct versions and paths as follows.

## Prerequisites

### Hardware
- Magic Leap 2 headset with **Developer Mode** enabled (via ML Hub)
- USB-C cable for ADB connection
- Magic Leap OS 12.1

### Version Reference

| Tool | Version |
|------|---------|
| Unity Editor | 2022.3 |
| Minimum API Level for Unity | 29 |
| Android SDK | 10 (Q) API Level 29 |
| Android NDK | v25.0.8775105 (known working); build.gradle specifies 26.2.11394342 |
| CMake | v3.22.1 |
| MLSDK | 12 (on ML Hub 3) |
| Java | 11 |
| Magic Leap OS | 12.1 |
| Gradle Plugin (AGP) | 7.4.2 |

### Software Installation

1. **Install the tools**
   - Install ML Hub 3, Unity Hub
   - Download the Unity Example projects (2.6)
   - Install the Unity Editor 2022.3 (NOTE: later versions may not work) and relevant modules (Android Build Support, Android SDK & NDK Tools, OpenJDK)

2. **Environment Setup for Android**
   - Follow the ML2 Tutorial on Unity, specifically 2 and 3.
   - NOTE: Download MLSDK version 1.12 from ML Hub 3.
   - NOTE: Ignore the setup for the ML2 application simulator.

3. **OpenCV Android SDK** — download and place at `CppPlugin/depth_navigation/OpenCV-android-sdk/`

### Environment Variables

These need to be added to your OS environment's path:

```bash
export MLSDK="$HOME/MagicLeap/mlsdk/v1.12.0"
export MAGICLEAP_APP_FRAMEWORK_ROOT="$MLSDK/samples/c_api/app_framework"  # NOTE: specific to C project
export ANDROID_HOME="$HOME/Library/Android/sdk"
export JAVA_HOME=/Library/Java/JavaVirtualMachines/openjdk-11.jdk/Contents/Home
```

### local.properties

Create a `local.properties` file in `CppPlugin/depth_navigation/` with your local SDK paths:

```properties
sdk.dir=/path/to/android/sdk
ndk.dir=/path/to/android/sdk/ndk/26.2.11394342
cmake.dir=/path/to/android/sdk/cmake/3.22.1
```

## C++ Project (`CppPlugin/depth_navigation/`)

The C++ project can be built and deployed independently of Unity as a standalone native Android app. OpenCV and the ML App Framework are co-located under `CppPlugin/`.

For release builds, configure `scripts/keystore.properties` (see `app/build.gradle`).

### Build, Install & Launch

All commands should be run from the `CppPlugin/depth_navigation/` directory.

**1. Verify Device Connection**

```sh
adb devices
```

Make sure your ML2 appears in the device list before proceeding.

**2. Build & Deploy (one command)**

```sh
./gradlew assembleMl2Debug && \
adb install -r app/build/outputs/apk/ml2/debug/com.magicleap.capi.sample.depth_camera-debug.apk && \
adb shell am start -S -n com.magicleap.capi.sample.depth_camera/android.app.NativeActivity
```

On first launch, grant the **Depth Camera** and **Camera** permissions when prompted.

> If `./gradlew` fails with a permission error, run `chmod +x gradlew` first.

## Debugging with Logcat

Use `adb logcat` with the `-s` flag to filter by tag. The app uses several log tags across its modules:

### Log Tags

| Tag | Source File | Description |
|-----|------------|-------------|
| `MarkerDetection` | `marker_detection.cpp` | IR blob detection results and parameters |
| `MarkerDepth` | `marker_detection.cpp` | Depth values at detected marker positions |
| `MarkerPair` | `marker_detection.cpp` | Pairwise distance calculations between markers |
| `ToolDef` | `tool_definition.cpp` | Tool registration, removal, and template info |
| `ToolTrack` | `tool_tracking.cpp` | Graph search matching and pose estimation |
| `com.magicleap.capi.sample.depth_camera` | `main.cpp`, `tool_definition.cpp`, `tool_tracking.cpp` | General app framework logs (ALOGI/ALOGE) |

### Usage

Filter to a single tag:
```sh
adb logcat -s ToolDef
```

Filter to multiple tags at once:
```sh
adb logcat -s MarkerDetection:I MarkerDepth:I ToolDef:I ToolTrack:V
```

Show all app-related logs:
```sh
adb logcat -s MarkerDetection:I MarkerDepth:I MarkerPair:I ToolDef:I ToolTrack:V com.magicleap.capi.sample.depth_camera:I
```

> The suffix (`:I`, `:V`, etc.) sets the minimum log level — `V`=verbose, `D`=debug, `I`=info, `E`=error.

## Uninstall

```sh
adb uninstall com.magicleap.capi.sample.depth_camera
```
