#!/usr/bin/env bash
# Generate compile_commands.json for depth_navigation using the Android CMake toolchain.
# Run from repo root or from CppPlugin/depth_navigation.
#
# Requires:
#   ANDROID_NDK  - Android NDK root (default: $HOME/Library/Android/sdk/ndk/26.2.11394342)
#   MLSDK        - Magic Leap SDK root (e.g. $HOME/MagicLeap/mlsdk/v1.12.0)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPTH_NAV_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CPP_DIR="$DEPTH_NAV_DIR/app/src/main/cpp"
BUILD_DIR="$DEPTH_NAV_DIR/app/build_compile_commands"
OUT_FILE="$DEPTH_NAV_DIR/compile_commands.json"

# Default NDK path on macOS if not set
if [[ -z "${ANDROID_NDK}" ]]; then
  if [[ -d "$HOME/Library/Android/sdk/ndk/26.2.11394342" ]]; then
    export ANDROID_NDK="$HOME/Library/Android/sdk/ndk/26.2.11394342"
  else
    echo "Error: ANDROID_NDK not set and default path not found. Set ANDROID_NDK to your NDK root."
    exit 1
  fi
fi

if [[ -z "${MLSDK}" ]]; then
  echo "Warning: MLSDK not set. CMake may fail on find_package(MagicLeap). Set MLSDK to your Magic Leap SDK root."
fi

NINJA=""
if command -v ninja &>/dev/null; then
  NINJA="-DCMAKE_MAKE_PROGRAM=$(command -v ninja) -GNinja"
fi

echo "Configuring CMake (Android x86_64) for depth_navigation..."
cmake \
  -S "$CPP_DIR" \
  -B "$BUILD_DIR" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=29 \
  -DANDROID_PLATFORM=android-29 \
  -DANDROID_ABI=x86_64 \
  -DCMAKE_ANDROID_ARCH_ABI=x86_64 \
  -DANDROID_NDK="$ANDROID_NDK" \
  -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  $NINJA

if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
  cp "$BUILD_DIR/compile_commands.json" "$OUT_FILE"
  echo "Created $OUT_FILE"
else
  echo "Error: compile_commands.json was not generated in $BUILD_DIR"
  exit 1
fi
