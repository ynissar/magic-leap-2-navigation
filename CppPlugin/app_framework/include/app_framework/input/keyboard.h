// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2022 Magic Leap, Inc. All Rights Reserved.
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
#include <app_framework/input/ml_input_handler.h>

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>

namespace ml {
namespace app_framework {

class Keyboard {
public:
  Keyboard(std::shared_ptr<ml::app_framework::MlInputHandler> keyboard_input_handler_);

  using OnKeyPress = std::function<void(int, int, bool)>;
  void Show(const OnKeyPress &key_press_callback);

  void Hide();

  bool Render();

  inline void SetHorizontalScroll(bool enable_horizontal_scroll) {
    horizontal_scroll_enabled_ = enable_horizontal_scroll;
  }

  inline const std::string &GetText() const {
    return text_;
  }

  inline void SetText(const std::string &text) {
    text_ = text;
  }

  inline bool IsOpen() const {
    return is_open_;
  }

  inline void SetEditBoxVisible(bool visible) {
    is_edit_box_visible_ = visible;
  }

  inline bool IsEditBoxVisible() const {
    return is_edit_box_visible_;
  }

private:
#ifndef ML_LUMIN
  static const int AKEYCODE_DEL = ImGuiKey_Delete;
  static const int AKEYCODE_ENTER = ImGuiKey_Enter;
  static const int AKEYCODE_SPACE = ImGuiKey_Space;
  static const int AKEYCODE_UNKNOWN = 0x0;
#endif
  bool horizontal_scroll_enabled_;
  bool is_edit_box_visible_;
  bool is_open_;
  std::function<void(uint32_t, int, bool)> key_press_callback_;
  std::shared_ptr<ml::app_framework::MlInputHandler> keyboard_input_handler_;
  bool shift_key_;
  std::string text_;
  float previous_scroll_max_;
};

}  // namespace app_framework
}  // namespace ml
