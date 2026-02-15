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

#include <android/log.h>

#define ALOG_VERBOSE ANDROID_LOG_VERBOSE
#define ALOG_FATAL   ANDROID_LOG_FATAL
#define ALOG_ERROR   ANDROID_LOG_ERROR
#define ALOG_DEBUG   ANDROID_LOG_DEBUG
#define ALOG_WARN    ANDROID_LOG_WARN
#define ALOG_INFO    ANDROID_LOG_INFO

#ifndef ALOG_TAG
#define ALOG_TAG "com.magicleap.capi.app_framework"
#endif

#ifndef ALOG
#define ALOG(priority, tag, ...) __android_log_print(priority, tag, __VA_ARGS__)
#endif

#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...) ((void)0)
#else
#define ALOGV(...) ((void)ALOG(ALOG_VERBOSE, ALOG_TAG, __VA_ARGS__))
#endif
#endif

#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(ALOG_DEBUG, ALOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(ALOG_INFO, ALOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(ALOG_WARN, ALOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(ALOG_ERROR, ALOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGA
#define ALOGA(...) ((void)ALOG(ALOG_FATAL, ALOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGF
#define ALOGF(...) __android_log_assert("assert", ALOG_TAG, __VA_ARGS__)
#endif

#define ALOG_IF(priority, output_log, ...) \
  if (output_log) {                        \
    ALOG(priority, ALOG_TAG, __VA_ARGS__); \
  }
