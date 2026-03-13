# %BANNER_BEGIN%
# ---------------------------------------------------------------------
# %COPYRIGHT_BEGIN%
# Copyright (c) 2022 Magic Leap, Inc. All Rights Reserved.
# Use of this file is governed by the Software License Agreement,
# located here: https://www.magicleap.com/software-license-agreement-ml2
# Terms and conditions applicable to third-party materials accompanying
# this distribution may also be found in the top-level NOTICE file
# appearing herein.
# %COPYRIGHT_END%
# ---------------------------------------------------------------------
# %BANNER_END%

cmake_minimum_required(VERSION 3.22.1)

project(depth_camera)

file(TO_CMAKE_PATH "$ENV{MLSDK}" MLSDK)
list(APPEND CMAKE_MODULE_PATH "${MLSDK}/cmake" "${ANDROID_NDK}/../../mlsdk/cmake")

# Set OpenCV directory (relative to this CMakeLists.txt file)
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../OpenCV-android-sdk/sdk/native/jni")
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")

find_package(MagicLeap REQUIRED)
find_package(MagicLeapAppFramework REQUIRED)

add_library(depth_camera SHARED
    main.cpp
    marker_detection.cpp
    tool_definition.cpp
    tool_tracking.cpp
)

include(DeprecatedApiUsage)
use_deprecated_api(depth_camera)

target_include_directories(depth_camera PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(depth_camera
    ML::app_framework
    ${OpenCV_LIBS}
)

if (COMMAND copy_artifacts)
    copy_artifacts(depth_camera)
endif()