# ML2 Setup

Since the ML2 is very reliant on its dependencies, it is important to have the correct versions and paths as follows.

## Version Reference

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

## Path

These need to be added to your OS environment's path as well. Below are some examples:

```bash
export MLSDK="$HOME/MagicLeap/mlsdk/v1.12.0"
export MAGICLEAP_APP_FRAMEWORK_ROOT="$MLSDK/samples/c_api/app_framework"  # NOTE: specific to C project
export ANDROID_HOME="$HOME/Library/Android/sdk"
export JAVA_HOME=/Library/Java/JavaVirtualMachines/openjdk-11.jdk/Contents/Home
```

## Setup Steps

1. **Install the tools**
   - Install ML Hub 3, Unity Hub
   - Download the Unity Example projects (2.6)
   - Install the Unity Editor 2022.3 (NOTE: later versions may not work) and relevant modules (Android Build Support, Android SDK & NDK Tools, OpenJDK)

2. **Environment Setup for Android**
   - Follow the ML2 Tutorial on Unity, specifically 2 and 3.
   - NOTE: Download MLSDK version 1.12 from ML Hub 3.
   - NOTE: Ignore the setup for the ML2 application simulator.

3. **C++ Project (`CppPlugin/depth_navigation/`)**
   - The C++ project can be built and deployed independently of Unity as a standalone native Android app.
   - OpenCV and the ML App Framework are co-located under `CppPlugin/`.
   - For release builds, configure `scripts/keystore.properties` (see `app/build.gradle`).
