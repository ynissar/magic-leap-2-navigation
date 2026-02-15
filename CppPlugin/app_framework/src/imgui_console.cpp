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

#include "imgui.h"
#include <app_framework/gui.h>
#include <app_framework/imgui_console.h>

namespace ml {
namespace app_framework {

ImGuiConsole::ImGuiConsole(ImGuiConsoleCloseCallback close_callback, bool visible_console, int max_lines) :
    close_callback_(close_callback),
    max_lines_(max_lines) {
      if (visible_console)
      {
        ml::app_framework::Gui::GetInstance().Show();
      }
    }

void ImGuiConsole::Draw() {
  Gui &gui = Gui::GetInstance();
  gui.BeginUpdate();
  bool is_running = true;
  if (gui.BeginDialog("Console", &is_running, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
    DrawChild(false);
  }
  gui.EndDialog();
  gui.EndUpdate();
  if (!is_running && close_callback_) {
    close_callback_();
  }
}

void ImGuiConsole::DrawChild(bool draw_header) {
  if (draw_header) {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Console");
  }
  ImGui::BeginChild("Output");
  {
    std::scoped_lock lock_(mutex_);
    for (const std::string &line : lines_) {
      ImGui::Text("%s", line.c_str());
    }
  }
  ImGui::EndChild();
}

void ImGuiConsole::Print(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  VPrint(format, arguments);
  va_end(arguments);
}

void ImGuiConsole::VPrint(const char *format, va_list args) {
  va_list arguments;

  // calculate the length of the string to allocate.

  va_copy(arguments, args);
  const int length = vsnprintf(NULL, 0, format, arguments) + 1;
  va_end(arguments);

  // allocate the string
  std::string s;
  s.resize(length);

  // 'print' to the string.
  va_copy(arguments, args);
  vsnprintf(&s[0], (size_t)length, format, arguments);
  va_end(arguments);

  {
    std::scoped_lock lock_(mutex_);
    // put in the string list buffer.
    lines_.push_back(s);
    while (lines_.size() > max_lines_) {
      lines_.pop_front();
    }
  }
}

void ImGuiConsole::Clear() {
  std::scoped_lock lock_(mutex_);
  lines_.clear();
}

}  // namespace app_framework
}  // namespace ml
