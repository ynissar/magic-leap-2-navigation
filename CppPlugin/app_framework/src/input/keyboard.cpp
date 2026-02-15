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

#include <app_framework/input/keyboard.h>
#include <app_framework/application.h>

namespace ml {
namespace app_framework {

Keyboard::Keyboard(std::shared_ptr <ml::app_framework::MlInputHandler> keyboard_input_handler) :
    horizontal_scroll_enabled_(false),
    is_edit_box_visible_(true),
    is_open_(false),
    key_press_callback_(nullptr),
    keyboard_input_handler_(keyboard_input_handler),
    shift_key_(false),
    text_(""),
    previous_scroll_max_(0) {}

void Keyboard::Show(const OnKeyPress &key_press_callback) {
  auto on_key_down = [this](ml::app_framework::InputHandler::EventArgs key_event_args) {
    if (key_press_callback_) {
      key_press_callback_(key_event_args.key_char, key_event_args.key_code,
                          key_event_args.key_modifier);
    }
  };

  auto on_char = [this](ml::app_framework::InputHandler::EventArgs key_event_args) {
    text_.push_back(key_event_args.key_char);
    if (key_press_callback_) {
      key_press_callback_(key_event_args.key_char, key_event_args.key_code,
                          key_event_args.key_modifier);
    }
  };

  auto on_key_up = [this](ml::app_framework::InputHandler::EventArgs key_event_args) {
    switch (key_event_args.key_code) {
      case AKEYCODE_DEL:
        if (!text_.empty()) {
          text_.pop_back();
        }
        break;
      case AKEYCODE_ENTER: is_open_ = false; break;
    }
    if (key_press_callback_) {
      key_press_callback_(key_event_args.key_char, key_event_args.key_code,
                          key_event_args.key_modifier);
    }
  };
  if (keyboard_input_handler_) {
    keyboard_input_handler_->SetEventHandlers(on_key_down, on_char, on_key_up);
  }

  key_press_callback_ = key_press_callback;
  shift_key_ = false;
  ImGui::OpenPopup("keyboard");
  is_open_ = true;
}

void Keyboard::Hide() {
  key_press_callback_ = nullptr;
  if (keyboard_input_handler_) {
    keyboard_input_handler_->SetEventHandlers(nullptr, nullptr, nullptr);
  }
}

bool Keyboard::Render() {
  if (!ImGui::BeginPopup("keyboard")) {
    is_open_ = false;
    return false;
  }
   if (is_edit_box_visible_) {
     if (!horizontal_scroll_enabled_) {
       ImGui::InputText("", text_.data(), text_.length() + 1, ImGuiInputTextFlags_ReadOnly);
     }
     else {
       // Use ImGui::Text and add horizontal scroll bar when text width gets too wide for window
       // Update scroll bar according to the max possible scroll from ImGui
       const auto text_w = ImGui::CalcTextSize(text_.c_str()).x;
       const auto window_w = ImGui::GetContentRegionAvail().x;
       const auto window_h =
         text_w > window_w ? (ImGui::GetTextLineHeight() * 1.2f + ImGui::GetStyle().ScrollbarSize) :
         ImGui::GetTextLineHeight() * 1.2f;

       ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
       ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ImGui::GetStyle().FrameRounding);
       ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, ImGui::GetStyle().FrameBorderSize);

       ImGui::BeginChild("inputtext", ImVec2(window_w, window_h), true,
                         ImGuiWindowFlags_HorizontalScrollbar);
       ImGui::PopStyleVar(2);
       ImGui::PopStyleColor();

       ImGui::Text("%s", text_.c_str());
       const float current_scroll = ImGui::GetScrollMaxX();
       if (previous_scroll_max_ != current_scroll) {
         ImGui::SetScrollX(current_scroll);
       }
       previous_scroll_max_ = current_scroll;

       ImGui::EndChild();
     }
  }

  // display each allowed character as a button
  std::string characters = "`1234567890-=\nqwertyuiop[]\\\nasdfghjkl;'\nzxcvbnm,./";
  if (shift_key_) {
    characters = "~!@#$%^&*()_+\nQWERTYUIOP{}|\nASDFGHJKL:\"\nZXCVBNM<>?";
  }

#ifndef ML_LUMIN
  const int AKEYCODE_SHIFT_LEFT = ImGui::GetIO().KeyShift;
#endif

  const int kShiftKeyCode = AKEYCODE_SHIFT_LEFT;
  const int kBackspaceKeyCode = AKEYCODE_DEL;
  const int kEnterKeyCode = AKEYCODE_ENTER;
  const int kSpaceKeyCode = AKEYCODE_SPACE;

  for (const char &c : characters) {
    if (c == '\n') {
      ImGui::NewLine();
      continue;
    }

    const std::string temp(1, c);
    if (ImGui::Button(temp.c_str())) {
      text_.push_back(c);

      if (key_press_callback_) {
        key_press_callback_(c, AKEYCODE_UNKNOWN, shift_key_);
      }
    }
    ImGui::SameLine();

    if (c == '=' || c == '+') {
      // backspace
      if (ImGui::Button("<-")) {
        if (!text_.empty()) {
          text_.pop_back();
        }

        if (key_press_callback_) {
          key_press_callback_(0, kBackspaceKeyCode, shift_key_);
        }
      }
      ImGui::SameLine();
    }

    if (c == '\'' || c == '\"') {
      // Enter/Go
      if (ImGui::Button("Go")) {
        ImGui::CloseCurrentPopup();

        if (key_press_callback_) {
          key_press_callback_(0, kEnterKeyCode, shift_key_);
        }
      }
      ImGui::SameLine();
    }
    if (!is_open_) ImGui::CloseCurrentPopup();
  }

  // Shift: control to switch between capital and small characters
  if (ImGui::Button("Shift")) {
    if (key_press_callback_) {
      key_press_callback_(0, kShiftKeyCode, shift_key_);
    }

    shift_key_ = !shift_key_;
  }

  if (ImGui::Button("                            ")) {
    text_ += " ";
    if (key_press_callback_) {
      key_press_callback_(' ', kSpaceKeyCode, shift_key_);
    }
  }

  ImGui::EndPopup();
  return true;
}
}  // namespace app_framework
}  // namespace ml
