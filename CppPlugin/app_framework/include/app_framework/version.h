// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2021 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#pragma once

#define ML_APP_FRAMEWORK_VERSION_MAJOR 0
#define ML_APP_FRAMEWORK_VERSION_MINOR 0
#define ML_APP_FRAMEWORK_VERSION_REVISION 0
#define ML_APP_FRAMEWORK_VERSION_BUILD_ID "0"
#define ML_APP_FRAMEWORK_STRINGIFYX(x) ML_APP_FRAMEWORK_STRINGIFY(x)
#define ML_APP_FRAMEWORK_STRINGIFY(x) #x
#define ML_APP_FRAMEWORK_VERSION_NAME \
  MLSDK_STRINGIFYX(ML_APP_FRAMEWORK_VERSION_MAJOR) \
  "." MLSDK_STRINGIFYX(ML_APP_FRAMEWORK_VERSION_MINOR) \
  "." MLSDK_STRINGIFYX(ML_APP_FRAMEWORK_VERSION_REVISION) \
  "." ML_APP_FRAMEWORK_VERSION_BUILD_ID
