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

#include <app_framework/common.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/graphics_context.h>
#include <app_framework/node.h>
#include <app_framework/render/texture.h>

#include "imgui.h"
#include <ml_head_tracking.h>
#include <ml_input.h>
#include <ml_types.h>

#include <chrono>

struct MLInputControllerStateEx;

namespace ml {
  class IAsset;
namespace app_framework {

/*! Dear ImGui integration into App Framework.

  Currently this class only supports a single instance to be active at
  any one time.

  To create an ImGui dialog use this code:

  \code{.cpp}
    void OnStart() override {
      auto & gui = ml::app_framework::Gui::GetInstance();
      gui.Initialize(GetGraphicsContext());
      GetRoot()->AddChild(gui.GetNode());
      gui.Show(); // places dialog relative to current head pose.
    }

    void OnUpdate(float) override {
      auto & gui = ml::app_framework::Gui::GetInstance();
      bool is_running = true;
      gui.BeginUpdate();
      if (gui.BeginDialog("ImGui Demo", &is_running)) {
        ImGui::BeginChild("Output");
        for ( const std::string & line : lines_ ) {
          ImGui::Text("%s", line.c_str());
        }
        ImGui::EndChild();
      }
      gui.EndDialog();
      gui.EndUpdate();
      if (!is_running) {
        FinishActivity();
      }
    }

    void OnStop() override {
      ml::app_framework::Gui::GetInstance().Cleanup();
    }
  \endcode

*/
class Gui final {
public:
  static Gui &GetInstance() {
    static Gui gui;
    return gui;
  }

  /*! Initializes the graphics and input handling.

    Call this once the graphics stack is ready to support
    OpenGL */
  void Initialize(GraphicsContext *g);

  /*! Cleans up graphics and input handling.*/
  void Cleanup();

  /*! Sets an external controller handle if a valid handle is sent else creates one internally.

    Returns the instance of the Gui object for command chaining.
  */
  Gui &SetControllerHandle(MLHandle handle);

  /*! Sets an external input handle if a valid handle is sent else creates one internally.

    Returns the instance of the Gui object for command chaining.
  */
  Gui &SetInputHandle(MLHandle handle);

  /*! BeginUpdate for the UI.

    Call this at the start of the user interface definition.

  */
  void BeginUpdate();

  /*! Resizes the ImGui dialog. */
  void Resize(float w, float h);

  /*! Renders the Gui to the GL surface. */
  void EndUpdate();

  /*! Places and shows the GUI at the specified location and rotation or pose. */
  void Place(const glm::quat &rotation, const glm::vec3 &translation, bool show_gui = true);
  void Place(const Pose &pose, bool show_gui = true);

  /*! Hides the GUI from view. */
  void Hide();

  /*! Shows the hidden GUI back in view. */
  void Show();

  /*! Returns the Node that the Gui is attached to. */
  std::shared_ptr<Node> GetNode() const {
    return root_node_;
  }

  /*! Returns the framebuffer the Gui is rendered on. */
  GLuint GetFrameBuffer() const {
    return imgui_framebuffer_;
  }

  /*! Returns the texture the Gui is rendered on. */
  std::shared_ptr<Texture> GetTexture() const {
    return off_screen_texture_;
  }

  /*! Sets the button that's used for changing Gui state. */
  void SetStateChangeButton(const MLInputControllerButton button);

#if !ML_LUMIN
  void DrawToWindow() const;
#endif

  /*! \brief BeginDialog used to start an ImGui dialog.

     Use this in place of ImGui::Begin to allow for extra functionality like 3d dragging.
  */
  bool BeginDialog(const char *name, bool *p_open = nullptr,
                   ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoCollapse);

  /*! \brief EndDialog used to finish building an ImGui dialog. */
  void EndDialog();

  /*! \brief Ray Configurations
   *
   * RAY_ALWAYS shows the ray when the gui is visible
   * RAY_NEAR_GUI shows the ray if the controller is pointing roughly at the gui
   * RAY_NEVER never shows the ray.
   */
  enum ControllerRayConfig { RAY_ALWAYS, RAY_NEAR_GUI, RAY_NEVER };
  /*! /brief sets the controller ray behavior */
  void SetControllerRayConfig(ControllerRayConfig config);

  /*! \brief Adds provided font into the ImGui's FontAtlas.

    Provided *font_buffer* should be valid for as long as the font is used.
    *is_owned* can be used to let ImGui take care of memory deallocation. ImGui uses free().
    *merge* can be used to merge provided *glyph_ranges* of given font into the currently loaded font.

    Note that ImGui DOES NOT support neither RTL, nor BiDi font rendering. Even if Arabic font is
    loaded it will still be displayed LTR. A quick hack is to reverse the unicode text before
    displaying it (so it will look like it was displayed in RTL).

    Returns pointer to the loaded font. Nullptr if loading failed.

    Example usage:

    \code{.cpp}
    AAsset* font_jpn_asset = LoadSomeAssetFile(); // E.x. "NotoSansJP-Medium.otf" loaded as an asset.
    AAsset* font_ara_asset = LoadSomeAssetFile(); // E.x. "NotoSansArabic-Medium.ttf" loaded as an asset.
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault(); // Add default font to merge the rest later into it

    auto font_jpn_len = AAsset_getLength(font_jpn_asset);
    void* font_buffer_jpn = malloc(font_jpn_len);
    AAsset_read(font_jpn_asset, font_buffer_jpn, font_jpn_len);

    auto font_ara_len = AAsset_getLength(font_ara_asset);
    void* font_buffer_ara = malloc(font_ara_len);
    AAsset_read(font_ara_asset, font_buffer_ara, font_ara_len);

    // We pass is_owned=true to let ImGuiFontAtlas deallocate font memory for us
    LoadFont(font_buffer_jpn, font_jpn_len, io.Fonts->GetGlyphRangesJapanese(), true, 20.f, true);
    LoadFont(font_buffer_ara, font_ara_len, Gui::GetGlyphRangesArabic(), true, 30.f, true);
    \endcode
  */
  ImFont* LoadFont(void* font_buffer, size_t font_buffer_len, const ImWchar* glyph_ranges = nullptr,
                   bool is_owned = false, float font_size = 20.f, bool merge = true);

  /*! \brief Loads font from provided asset and adds loaded font into the ImGui's FontAtlas.

    Returns pointer to the loaded font. Nullptr if loading failed.
  */
  ImFont* LoadFont(std::shared_ptr<IAsset> font_asset, const ImWchar* glyph_ranges = nullptr,
                   float font_size = 20.f, bool merge = true);

  /*! \brief Subset of Arabic glyph ranges, as ImGui does not provide those. */
  static const ImWchar* GetGlyphRangesArabic();

private:
  Gui();
  Gui(const Gui &other) = delete;
  Gui(Gui &&other) = delete;
  Gui &operator=(const Gui &other) = delete;
  Gui &operator=(Gui &&other) = delete;
  bool initialized_;
  std::shared_ptr<Texture> off_screen_texture_;
  MLHandle input_handle_;
  MLHandle controller_handle_;
  MLVec3f prev_touch_pos_and_force_;
  glm::vec2 cursor_pos_;
  enum class State { Hidden, Moving, Placed } state_;
  std::shared_ptr<Node> root_node_;
  std::shared_ptr<Node> gui_node_;
  std::shared_ptr<Node> gui_mesh_node_;
  std::shared_ptr<Node> ray_node_;
  std::shared_ptr<Node> ray_mesh_node_;
  bool prev_toggle_state_;
  MLInputControllerButton state_change_button_;
  GLuint imgui_color_texture_;
  GLuint imgui_framebuffer_;
  GLuint imgui_depth_renderbuffer_;
  std::chrono::steady_clock::time_point previous_time_;
  std::shared_ptr<QuadMesh> gui_mesh_quad_;
  Pose controller_pose_;
  typedef struct DragInfo {
    glm::vec3 global_ray_gui_intersect;
    Pose gui_start;
    Pose controller_start;
    std::chrono::steady_clock::time_point drag_begin;
    DragInfo() :
        global_ray_gui_intersect(glm::vec3(0.0f, 0.0f, 0.0f)),
        drag_begin(std::chrono::steady_clock::time_point()) {}
  } DragInfo;
  DragInfo drag_info_;
  // Current (or last) point in World Space where the
  // cursor is intercepting the GUI.
  glm::vec3 global_ray_gui_intersect_;
  int32_t gui_bounds_w_, gui_bounds_h_;
  // Controls how the controller ray behaves.
  ControllerRayConfig controller_ray_config_;

  void UpdateState(const MLInputControllerStateEx &input_state);
  void UpdateMousePosAndButtons(const MLInputControllerStateEx &input_state, bool has_controller_pose);
  // returns true if controller has pose, false otherwise.
  bool UpdateControllerPose();
  void UpdateRayPose();
  void MoveGui(const glm::quat &rotation, const glm::vec3 &translation);

  // State Machine entry point.
  // Don't change state_ directly call SetState.
  void SetState(const State new_state);
  // Don't call these directly, they are for the state machine
  void MakeVisible();
  void MakeInvisible();
  void StartMoving();
  void KeepMoving();
  void StopMoving();
  // End State Machine.

#if !ML_LUMIN
  struct GLFWwindow *window_;
  bool glfw_window_is_initialized_;
#endif
};
}  // namespace app_framework
}  // namespace ml
