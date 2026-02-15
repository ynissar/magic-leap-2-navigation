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

#include <android_native_app_glue.h>
#include <app_framework/input/input_handler.h>

namespace ml {
namespace app_framework {

class MlInputHandler : public InputHandler {
public:
  /* method called when a key press is detected */
  void OnKeyDown(int32_t key_code, uint32_t key_modifier);
  /* method called with unicode char value of the key pressed */
  void OnChar(uint32_t key_value);
  /* method called when the key is released */
  void OnKeyUp(int32_t key_code, uint32_t key_modifier);
};

}  // namespace app_framework
}  // namespace ml
