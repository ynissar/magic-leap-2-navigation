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
#include <app_framework/input/ml_input_handler.h>

namespace ml {
namespace app_framework {

void MlInputHandler::OnKeyDown(int32_t key_code, uint32_t key_modifier) {
  auto callback = this->GetOnKeyDownCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {0, static_cast<uint32_t>(key_code), key_modifier};
  callback(key_event_args);
}

void MlInputHandler::OnChar(uint32_t key_value) {
#if ML_LUMIN
  auto callback = this->GetOnCharCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {key_value, AKEYCODE_UNKNOWN, 0};
  callback(key_event_args);
#else
  (void)key_value;
#endif
}

void MlInputHandler::OnKeyUp(int32_t key_code, uint32_t key_modifier) {
#if ML_LUMIN
  auto callback = this->GetOnKeyUpCallback();
  if (!callback) {
    return;
  }

  EventArgs key_event_args = {0, static_cast<uint32_t>(key_code), key_modifier};
  callback(key_event_args);
#else
  (void)key_code;
  (void)key_modifier;
#endif
}

}  // namespace app_framework
}  // namespace ml
