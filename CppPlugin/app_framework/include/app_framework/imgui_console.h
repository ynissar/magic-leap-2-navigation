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

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <stdarg.h>

namespace ml {
namespace app_framework {

/*! \brief Callback used when the user clicks the "X" in the top right hand corner. */
typedef std::function<void()> ImGuiConsoleCloseCallback;

/*! \brief Virtual text output for easily printing text in 3d space.

    #Application uses this class to provide an easy to use
    virtual text output in 3d space. You can either call #Application::GetConsole or
    create this directly.

    Note that this uses #Gui under the hood and there can only be
    one instance of that class in any process.
    */
class ImGuiConsole {
public:
  /*! \brief ImGuiConsole constructor.

      \param close_callback to specify the function to run when close button is clicked.
      \param max_lines is the maximum number of lines that are displayed in the console.
      \param visible_console allows the user to hide the console on construction. Default to be visible.
  */
  ImGuiConsole(ImGuiConsoleCloseCallback close_callback, bool visible_console = true, int max_lines = 28);

  /*! \brief Prints into the Gui console.

    Uses same syntax as printf.
    Note: thread-safe.

   */
  void Print(const char *format, ...);

  /*! \brief Prints into the Gui console.

    Uses same syntax as vprintf.
    Note: thread-safe.

   */
  void VPrint(const char *format, va_list args);

  /*! \brief Clears the Gui console.

      Note: thread-safe.
   */
  void Clear();

  /*! \brief Function to be called in the OnUpdate lifecycle callback. If
     another ImGui dialogue is present then embed the UI inside the existing
     dialogue by using DrawChild()
   */
  void Draw();

  /*! \brief Function to be called to embed the console into any ImGui rendering function.

    \param drawHeader explicitly prints a console box to be embedded into the
    ImGui dialogue. If false then the "Console header is not specified"
   */
  void DrawChild(bool draw_header = true);

private:
  ImGuiConsoleCloseCallback close_callback_;
  std::list<std::string> lines_;
  uint32_t max_lines_;
  std::mutex mutex_;
};

/*! \brief Shared pointer to ImGuiConsole. */
typedef std::shared_ptr<ImGuiConsole> ImGuiConsolePtr;

}  // namespace app_framework
}  // namespace ml
