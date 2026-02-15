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
#include "app_framework/gui.h"

#include <app_framework/asset_manager.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/convert.h>
#include <app_framework/ml_macros.h>
#include <app_framework/render/texture.h>
#include <app_framework/toolset.h>

#include <app_framework/components/light_component.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/material/flat_material.h>
#include <app_framework/material/textured_material.h>

#if !ML_LUMIN
#include <GLFW/glfw3.h>
#include <app_framework/imgui/imgui_impl_glfw.h>
#endif

#include <app_framework/imgui/imgui_impl_opengl3.h>
#include <ml_controller.h>
#include <ml_perception.h>
#include <ml_snapshot.h>

#include <sstream>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/intersect.hpp>

namespace ml {
namespace app_framework {

static constexpr int32_t kImguiTextureWidth = 540;
static constexpr int32_t kImguiTextureHeight = 540;
static constexpr float kImguiCanvasScale = 0.75f;
static constexpr float kCursorSpeed = kImguiTextureWidth * 10.f;
static constexpr float kPressThreshold = 0.5f;

// ImGUI context really only wants to be created once per process.
// This class makes sure of that.
class ImGuiContext {
public:
  static ImGuiContext &GetInstance() {
    static ImGuiContext ctx;
    return ctx;
  }

private:
  ImGuiContext() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
  }
  ~ImGuiContext() {
    ImGui::DestroyContext();
  }
};

Gui::Gui() :
    initialized_(false),
    input_handle_(ML_INVALID_HANDLE),
    controller_handle_(ML_INVALID_HANDLE),
    prev_touch_pos_and_force_{},
    cursor_pos_{},
    state_(State::Hidden),
    prev_toggle_state_(false),
    state_change_button_(MLInputControllerButton_Bumper),
    imgui_color_texture_(0),
    imgui_framebuffer_(0),
    imgui_depth_renderbuffer_(0),
    gui_bounds_w_(kImguiTextureWidth),
    gui_bounds_h_(kImguiTextureHeight),
    controller_ray_config_(RAY_NEAR_GUI)
#if !ML_LUMIN
    ,
    window_(nullptr),
    glfw_window_is_initialized_(false)
#endif
{
  ImGuiContext::GetInstance();
}

void Gui::Initialize(GraphicsContext *g) {
  (void)g;

  if (initialized_) {
#ifdef ML_LUMIN
    ALOGF("Gui::Initialize called a second time in the same process. "
          "ImGui only allows one instance per shared object.");
#else
    ALOGE("Gui::Initialize called a second time in the same process. "
          "ImGui only allows one instance per shared object.");
    exit(-1);
#endif
  };

  ImGui_ImplOpenGL3_Init("#version 410 core");
#if ML_LUMIN
  // Setup back-end capabilities flags
  ImGuiIO &io = ImGui::GetIO();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
  io.KeyMap[ImGuiKey_Tab] = AKEYCODE_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = AKEYCODE_DPAD_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = AKEYCODE_DPAD_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = AKEYCODE_DPAD_UP;
  io.KeyMap[ImGuiKey_DownArrow] = AKEYCODE_DPAD_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = AKEYCODE_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = AKEYCODE_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = AKEYCODE_HOME;
  io.KeyMap[ImGuiKey_End] = AKEYCODE_ENDCALL;
  io.KeyMap[ImGuiKey_Insert] = AKEYCODE_INSERT;
  io.KeyMap[ImGuiKey_Delete] = AKEYCODE_DEL;
  io.KeyMap[ImGuiKey_Backspace] = AKEYCODE_BACK;
  io.KeyMap[ImGuiKey_Space] = AKEYCODE_SPACE;
  io.KeyMap[ImGuiKey_Enter] = AKEYCODE_ENTER;
  io.KeyMap[ImGuiKey_Escape] = AKEYCODE_ESCAPE;
  io.KeyMap[ImGuiKey_A] = AKEYCODE_A;
  io.KeyMap[ImGuiKey_C] = AKEYCODE_C;
  io.KeyMap[ImGuiKey_V] = AKEYCODE_V;
  io.KeyMap[ImGuiKey_X] = AKEYCODE_X;
  io.KeyMap[ImGuiKey_Y] = AKEYCODE_Y;
  io.KeyMap[ImGuiKey_Z] = AKEYCODE_Z;
#else
  window_ = (GLFWwindow *)g->GetDisplayHandle();
  if (window_) {
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    glfw_window_is_initialized_ = true;
  }
#endif

  glGenFramebuffers(1, &imgui_framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);

  glGenTextures(1, &imgui_color_texture_);
  glBindTexture(GL_TEXTURE_2D, imgui_color_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kImguiTextureWidth, kImguiTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  off_screen_texture_ = std::make_shared<Texture>(GL_TEXTURE_2D, imgui_color_texture_, kImguiTextureWidth,
                                                  kImguiTextureHeight, false);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imgui_color_texture_, 0);

  glGenRenderbuffers(1, &imgui_depth_renderbuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, imgui_depth_renderbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, kImguiTextureWidth, kImguiTextureHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, imgui_depth_renderbuffer_);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    ALOGA("Framebuffer is not complete!");
  }
  ImGui::StyleColorsDark();

  gui_mesh_quad_ = std::make_shared<QuadMesh>();
  std::shared_ptr<TexturedMaterial> gui_mat = std::make_shared<TexturedMaterial>(off_screen_texture_);
  gui_mat->SetRemoveAlpha(true);
  gui_mat->EnableAlphaBlending(false);
  gui_mat->SetPolygonMode(GL_FILL);
  std::shared_ptr<RenderableComponent> gui_renderable = std::make_shared<RenderableComponent>(gui_mesh_quad_, gui_mat);

  gui_node_ = std::make_shared<Node>();
  gui_mesh_node_ = std::make_shared<Node>();
  gui_mesh_node_->AddComponent(gui_renderable);
  gui_mesh_node_->SetLocalScale(glm::vec3(kImguiCanvasScale, kImguiCanvasScale, 0.1f));
  gui_node_->AddChild(gui_mesh_node_);

  ray_node_ = std::make_shared<Node>();

  ray_mesh_node_ = CreatePresetNode(NodeType::Cube);
  auto material = std::static_pointer_cast<FlatMaterial>(
      ray_mesh_node_->GetComponent<RenderableComponent>()->GetMaterial());
  material->EnableAlphaBlending(true);
  material->SetPolygonMode(GL_FILL);
  if (controller_ray_config_ == RAY_ALWAYS) {
    ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true);
  }
  ray_mesh_node_->SetLocalScale(glm::vec3(0.0011f, 0.0011f, 1.0f));
  ray_mesh_node_->SetLocalTranslation(glm::vec3(0.0f, 0.0f, -0.55f));  // start 5cm away from the controller.

  root_node_ = std::make_shared<Node>();
#if ML_LUMIN
  root_node_->AddChild(gui_node_);
  ray_node_->AddChild(ray_mesh_node_);
  root_node_->AddChild(ray_node_);
#endif
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiTextureWidth), static_cast<float>(kImguiTextureHeight)),
                           ImGuiCond_FirstUseEver);
  gui_bounds_w_ = kImguiTextureWidth;
  gui_bounds_h_ = kImguiTextureHeight;

  switch (state_) {
    case State::Hidden: MakeInvisible(); break;
    default: MakeVisible();
  }

  initialized_ = true;
}

void Gui::Cleanup() {
  if (!initialized_) {
    return;
  }

  state_ = State::Hidden;
  glDeleteTextures(1, &imgui_color_texture_);
  glDeleteFramebuffers(1, &imgui_framebuffer_);
  glDeleteRenderbuffers(1, &imgui_depth_renderbuffer_);

  ImGui_ImplOpenGL3_Shutdown();

#if !ML_LUMIN
  if (window_) {
    ImGui_ImplGlfw_Shutdown();
  }
#endif

  initialized_ = false;
}

Gui &Gui::SetControllerHandle(MLHandle handle) {
  if (handle == ML_INVALID_HANDLE) {
    ALOGE("Cannot pass ML_INVALID_HANDLE");
    return *this;
  }
  controller_handle_ = handle;
  return *this;
}

Gui &Gui::SetInputHandle(MLHandle handle) {
  if (handle == ML_INVALID_HANDLE) {
    ALOGE("Cannot pass ML_INVALID_HANDLE");
    return *this;
  }
  input_handle_ = handle;
  return *this;
}

void Gui::BeginUpdate() {
  if (!initialized_) {
    return;
  }
  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = ImVec2((float)kImguiTextureWidth, (float)kImguiTextureHeight);
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // Setup time step
  auto current_time = std::chrono::steady_clock::now();
  std::chrono::duration<float> delta_time = current_time - previous_time_;
  io.DeltaTime = delta_time.count();
  previous_time_ = current_time;
  io.MouseDrawCursor = true;

  // Start the ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  IM_ASSERT(io.Fonts->IsBuilt());

#if ML_LUMIN
  // Handle input
  MLInputControllerStateEx input_states[MLInput_MaxControllers] = {};
  MLInputControllerStateExInit(input_states);
  MLInputGetControllerStateEx(input_handle_, input_states);
  UpdateState(input_states[0]);
#else
  if (window_) {
    // UpdateMousePosAndButtons();
    ImGui_ImplGlfw_NewFrame();
  }
#endif

  ImGui::NewFrame();

  // Set default position and size for new windows
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kImguiTextureWidth), static_cast<float>(kImguiTextureHeight)),
                           ImGuiCond_FirstUseEver);
}

void Gui::Resize(float w, float h) {
  if (gui_mesh_quad_ && (gui_bounds_w_ != w || gui_bounds_h_ != h)) {
    gui_bounds_w_ = static_cast<int32_t>(w);
    gui_bounds_h_ = static_cast<int32_t>(h);
    const float scaled_w = w / static_cast<float>(kImguiTextureWidth);
    const float scaled_h = h / static_cast<float>(kImguiTextureHeight);
    gui_mesh_node_->SetLocalScale(glm::vec3(kImguiCanvasScale * scaled_w, kImguiCanvasScale * scaled_h, 0.1f));

    const float inverted_scaled_h = 1.f - scaled_h;
    const std::array<glm::vec2, 8> scaled_tex_uvs = {{
        {0.0f, inverted_scaled_h},
        {0.0f, 1.f},
        {scaled_w, inverted_scaled_h},
        {scaled_w, 1.f},
    }};
    gui_mesh_quad_->UpdateTexCoordsBuffer(scaled_tex_uvs.data());
  }
}

void Gui::EndUpdate() {
  if (!initialized_) {
    return;
  }

  ImGui::Render();
  // Off-screen render
  glBindFramebuffer(GL_FRAMEBUFFER, imgui_framebuffer_);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  auto draw_data = ImGui::GetDrawData();
  if (draw_data != nullptr) {
    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#if !ML_LUMIN
void Gui::DrawToWindow() const {
  if (true == glfw_window_is_initialized_) {
    auto draw_data = ImGui::GetDrawData();
    if (draw_data != nullptr) {
      ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }
  }
}
#endif

void Gui::UpdateState(const MLInputControllerStateEx &input_state) {
  bool has_controller_pose = UpdateControllerPose();

  if (has_controller_pose) {
    UpdateRayPose();
  } else {
    ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(false);
  }

  if (state_ != State::Hidden) {
    UpdateMousePosAndButtons(input_state, has_controller_pose);
  }

  if (state_ != State::Moving) {
    // toggle visibility with the special state_change_button.
    const bool toggle_state = input_state.button_state[state_change_button_];
    if (toggle_state && !prev_toggle_state_) {
      switch (state_) {
        case State::Hidden: SetState(State::Placed); break;
        case State::Placed: SetState(State::Hidden); break;
        case State::Moving: break;
      }
    }
    prev_toggle_state_ = toggle_state;
  }
}

bool Gui::UpdateControllerPose() {
  if (controller_handle_ == ML_INVALID_HANDLE) {
    return false;
  }
  bool result = false;
  MLSnapshot *snapshot = nullptr;
  UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));

  MLControllerSystemState system_state = {};
  std::stringstream status;
  UNWRAP_MLRESULT(MLControllerGetState(controller_handle_, &system_state));

  MLInputControllerStateEx controller_states[MLInput_MaxControllers];
  MLInputControllerStateExInit(controller_states);

  UNWRAP_MLRESULT(MLInputGetControllerStateEx(input_handle_, controller_states));

  if (controller_states[0].is_connected &&
      system_state.controller_state[0].stream[MLControllerMode_Fused6Dof].is_active) {
    MLTransform controller_transform;
    MLSnapshotGetTransform(snapshot,
                           &system_state.controller_state[0].stream[MLControllerMode_Fused6Dof].coord_frame_controller,
                           &controller_transform);
    controller_pose_ = controller_transform;
    result = true;
  }
  UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
  return result;
}

void Gui::UpdateRayPose() {
  ray_node_->SetLocalPose(controller_pose_);
}

void Gui::UpdateMousePosAndButtons(const MLInputControllerStateEx &input_state, bool has_controller_pose) {
  const glm::vec2 gui_bounds = glm::vec2(gui_bounds_w_, gui_bounds_h_);

  // Calculate the 6dof Ray ImgGui interception and place cursor in the appropriate
  // spot.
  if (has_controller_pose) {
    glm::vec3 ray_dir(0.0f, 0.0f, -1.f);
    glm::vec3 plane_normal(0.f, 0.f, 1.f);

    const glm::mat4 ray_2_world = ray_node_->GetWorldTransform();
    const glm::mat4 gui_2_world = gui_mesh_node_->GetWorldTransform();

    ray_dir = glm::vec3(ray_2_world * glm::vec4(ray_dir, 0.0f));
    plane_normal = glm::vec3(gui_2_world * glm::vec4(plane_normal, 0.0f));

    glm::vec3 ray_center = ray_node_->GetWorldTranslation();
    glm::vec3 plane_center = gui_mesh_node_->GetWorldTranslation();

    ray_dir = glm::normalize(ray_dir);
    plane_normal = glm::normalize(plane_normal);

    float distance = 0.f;
    bool has_intersected = glm::intersectRayPlane(ray_center, ray_dir, plane_center, plane_normal, distance);
    if (has_intersected) {
      glm::vec3 intersect = ray_center + ray_dir * distance;
      global_ray_gui_intersect_ = intersect;
      glm::vec3 local = glm::vec3(glm::inverse(gui_2_world) * glm::vec4(intersect, 1.0f));
      const float edge = 0.6f;
      if (!(local[0] < -edge || local[0] > edge || local[1] < -edge || local[1] > edge)) {
        cursor_pos_ = glm::clamp(glm::vec2((local[0] + 0.5f) * gui_bounds.x, (-local[1] + 0.5f) * gui_bounds.y),
                                 glm::vec2(0), gui_bounds);
        if (controller_ray_config_ == RAY_NEAR_GUI) {
          ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true);
        }
      } else {
        if (controller_ray_config_ == RAY_NEAR_GUI) {
          ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(false);
        }
      }
    }
  }

  // We're dragging the window!
  if (state_ == State::Moving) {
    if (input_state.trigger_normalized < kPressThreshold) {
      SetState(State::Placed);
    } else {
      SetState(State::Moving);  // keep on moving.
    }
    // if we are moving don't interact with the GUI.
    return;
  }

  // Update buttons and handle the trackpad.
  ImGuiIO &io = ImGui::GetIO();

  bool prev_mouse_down[5] = {};
  std::memcpy(prev_mouse_down, io.MouseDown, 5);

  io.MouseDown[0] = input_state.trigger_normalized > kPressThreshold;

  if (input_state.touch_pos_and_force[0].z > 0.f && prev_touch_pos_and_force_.z > 0.f) {
    float delta_x = (input_state.touch_pos_and_force[0].x - prev_touch_pos_and_force_.x);
    float delta_y = (input_state.touch_pos_and_force[0].y - prev_touch_pos_and_force_.y);
    glm::vec2 delta_pos = io.DeltaTime * kCursorSpeed * glm::vec2(delta_x, -delta_y);

    cursor_pos_ = glm::clamp(cursor_pos_ + delta_pos, glm::vec2(0), gui_bounds);
  }
  io.MousePos = ImVec2(cursor_pos_.x, cursor_pos_.y);
  prev_touch_pos_and_force_ = input_state.touch_pos_and_force[0];
}

void Gui::Place(const glm::quat &rotation, const glm::vec3 &translation, bool show_gui) {
  if (show_gui) {
    Show();
  }
  MoveGui(rotation, translation);
}

void Gui::Place(const Pose &pose, bool show_gui) {
  Place(pose.rotation, pose.position, show_gui);
}

void Gui::Hide() {
  switch (controller_ray_config_) {
    case RAY_ALWAYS: [[fallthrough]];
    case RAY_NEAR_GUI: ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(false); break;
    case RAY_NEVER: break;
  }
  SetState(State::Hidden);
}

void Gui::Show() {
  switch (controller_ray_config_) {
    case RAY_ALWAYS: [[fallthrough]];
    case RAY_NEAR_GUI: ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true); break;
    case RAY_NEVER: break;
  }

  SetState(State::Placed);
}

void Gui::SetStateChangeButton(const MLInputControllerButton button) {
  state_change_button_ = button;
}

bool Gui::BeginDialog(const char *name, bool *p_open, ImGuiWindowFlags flags) {
  bool result = ImGui::Begin(name, p_open, flags);
  if (ImGui::IsItemClicked()) {
    SetState(State::Moving);
  }
  return result;
}

void Gui::EndDialog() {
  Resize(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
  ImGui::End();
}

void Gui::StartMoving() {
  gui_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true);
  drag_info_.global_ray_gui_intersect = global_ray_gui_intersect_;
  drag_info_.gui_start = gui_node_->GetWorldPose();
  drag_info_.controller_start = controller_pose_;
  drag_info_.drag_begin = std::chrono::steady_clock::now();
  KeepMoving();  // let's start moving!
}

void Gui::KeepMoving() {
  const auto &di = drag_info_;

  glm::quat controller_rotation_difference = controller_pose_.rotation * glm::inverse(di.controller_start.rotation);

  glm::quat rotation = controller_rotation_difference * di.gui_start.rotation;
  glm::vec3 center_offset = di.global_ray_gui_intersect - di.gui_start.position;
  glm::vec3 ray = di.global_ray_gui_intersect - di.controller_start.position;

  glm::vec3 offset = controller_pose_.position - controller_rotation_difference * center_offset +
                     controller_rotation_difference * ray;
  MoveGui(rotation, offset);
}

void Gui::StopMoving() {
  KeepMoving();  // just one more time.
}

void Gui::MakeVisible() {
  gui_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true);
}

void Gui::MakeInvisible() {
  gui_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(false);
}

void Gui::SetState(const State new_state) {
  const State previous = state_;
  switch (previous) {
    case State::Hidden:
      switch (new_state) {
        case State::Moving: StartMoving(); break;
        case State::Placed: MakeVisible(); break;
        case State::Hidden: break;
      }
      break;

    case State::Placed:
      switch (new_state) {
        case State::Moving: StartMoving(); break;
        case State::Placed: break;
        case State::Hidden: MakeInvisible(); break;
      }
      break;

    case State::Moving:
      switch (new_state) {
        case State::Moving: KeepMoving(); break;
        case State::Placed: StopMoving(); break;
        case State::Hidden: MakeInvisible(); break;
      }
      break;
  }
  state_ = new_state;
}

void Gui::MoveGui(const glm::quat &rotation, const glm::vec3 &translation) {
  gui_node_->SetLocalRotation(rotation);
  gui_node_->SetLocalTranslation(translation);
}

void Gui::SetControllerRayConfig(ControllerRayConfig config) {
  controller_ray_config_ = config;
  switch (controller_ray_config_) {
    case RAY_ALWAYS: ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(true); break;
    case RAY_NEVER: ray_mesh_node_->GetComponent<RenderableComponent>()->SetVisible(false); break;
    case RAY_NEAR_GUI:
      // nothing to do.
      break;
  }
}

ImFont* Gui::LoadFont(void* font_buffer, size_t font_buffer_len, const ImWchar* glyph_ranges, bool is_owned, float font_size, bool merge) {
  if (font_buffer == nullptr || font_buffer_len <= 0) {
    ALOGE("Passed invalid font buffer or length!");
    return nullptr;
  }

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig cfg;
  cfg.FontDataOwnedByAtlas = is_owned; // This can be used so that ImGui frees memory for the font
  cfg.MergeMode = merge; // This will merge loaded glyph ranges into an already existing font
  return io.Fonts->AddFontFromMemoryTTF(font_buffer, font_buffer_len, font_size, &cfg, glyph_ranges);
}

ImFont* Gui::LoadFont(std::shared_ptr<IAsset> font_asset, const ImWchar* glyph_ranges, float font_size, bool merge) {
  if (!font_asset) {
    ALOGE("Passed invalid font asset!");
    return nullptr;
  }

  try {
    const auto len = font_asset->Length();
    void* font_buffer = malloc(len);
    font_asset->Read(font_buffer, len);

    return LoadFont(font_buffer, len, glyph_ranges, true, font_size, merge);
  } catch (const std::runtime_error& ex) {
    ALOGE("Encountered runtime_error while accessing font asset!");
    return nullptr;
  }
}

const ImWchar* Gui::GetGlyphRangesArabic()
{
  // These are not ALL Arabic glyph ranges! Just some subset based on unicode webpage
  static const ImWchar ranges[] =
  {
      0x0600, 0x06FF, // Arabic
      0x0750, 0x077F, // Arabic Supplement
      0x08A0, 0x08FF, // Arabic Extended-A
      0x0870, 0x089F, // Arabic Extended-B
      0,
  };
  return &ranges[0];
}

}  // namespace app_framework
}  // namespace ml
