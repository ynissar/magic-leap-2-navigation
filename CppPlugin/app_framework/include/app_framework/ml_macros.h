// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#pragma once

#include "logging.h"

#define MLRESULT_EXPECT(result, expected, string_fn, level)                                              \
  do {                                                                                                   \
    MLResult _evaluated_result = result;                                                                 \
    ALOG_IF(level, (expected) != _evaluated_result, "%s:%d %s (%X)%s", __FILE__, __LINE__, __FUNCTION__, \
            _evaluated_result, (string_fn)(_evaluated_result));                                          \
  } while (0)

#define UNWRAP_RET_MLRESULT_GENERIC(result, unwrap_mcr) \
  {                                                     \
    const MLResult __temp = result;                     \
    unwrap_mcr(__temp);                                 \
    if (__temp != MLResult_Ok) return __temp;           \
  }

#define ASSERT_MLRESULT_GENERIC(result, unwrap_mcr) \
  {                                                 \
    const MLResult __temp = result;                 \
    unwrap_mcr(__temp);                             \
    if (__temp != MLResult_Ok) FinishActivity();    \
  }

#define UNWRAP_MLRESULT(result)       MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, ALOG_ERROR)
#define UNWRAP_MLRESULT_FATAL(result) MLRESULT_EXPECT((result), MLResult_Ok, MLGetResultString, ALOG_FATAL)
#define UNWRAP_RET_MLRESULT(result)   UNWRAP_RET_MLRESULT_GENERIC(result, UNWRAP_MLRESULT)
#define ASSERT_MLRESULT(result)       ASSERT_MLRESULT_GENERIC(result, UNWRAP_MLRESULT)

#define UNWRAP_MLMEDIA_RESULT(result)    MLRESULT_EXPECT((result), MLResult_Ok, MLMediaResultGetString, ALOG_ERROR)
#define UNWRAP_MLINPUT_RESULT(result)    MLRESULT_EXPECT((result), MLResult_Ok, MLInputGetResultString, ALOG_ERROR)
#define UNWRAP_MLSNAPSHOT_RESULT(result) MLRESULT_EXPECT((result), MLResult_Ok, MLSnapshotGetResultString, ALOG_ERROR)
