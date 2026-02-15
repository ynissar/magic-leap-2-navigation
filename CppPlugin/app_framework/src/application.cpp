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
#include <app_framework/application.h>
#include <app_framework/asset_manager.h>
#include <app_framework/cli_args_parser.h>
#include <app_framework/convert.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/gui.h>
#include <app_framework/input/keyboard.h>
#include <app_framework/input/ml_input_handler.h>
#include <app_framework/material/textured_material.h>
#include <app_framework/ml_macros.h>
#include <app_framework/render/render_target.h>
#include <app_framework/render/renderer.h>

#if !ML_LUMIN
#include <GLFW/glfw3.h>
#endif

#include <gflags/gflags.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/quaternion.hpp>

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <vector>

#include <ml_controller.h>
#include <ml_perception.h>

namespace chrono = std::chrono;

namespace ml {
namespace app_framework {

const std::string Application::kEmptyStr = "";
std::mutex Application::graphics_mutex_;

DEFINE_int32(gl_debug_severity, 2,
             "Enable OpenGL logging for messages up to and including the given severity.\n"
             "0: Off, 1: High, 2: Medium, 3: Low, 4: Notification");

DEFINE_bool(mirror_window, true,
            "In addition to streaming graphics to MLRemote, also make a native window to mirror the display");

DEFINE_int32(window_width, 800, "Width of the window on the host machine.");

DEFINE_int32(window_height, 600, "Height of the window on the host machine.");

DEFINE_double(perf_log_rate, 0,
              "How often to log the performance information in seconds. A value of 0 will prevent logging this "
              "performance "
              "information at all. This also requires LogLevel to be 4 or greater.");

#if ML_WINDOWS && ML_USE_HIGH_PERFORMANCE_GPU

// enable Nvidia and AMD specific symbols for high performance GPU
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

#endif

// the type must be lock-free
std::atomic<bool> Application::exit_signal_;

Application::Application(struct android_app *app, std::vector<std::string> permissions, int flags) :
    android_app_(app),
    android_app_visible_(false),
    android_app_focussed_(false),
    use_gui_((flags & USE_GUI) ? true : false),
    use_gfx_((flags & NO_OPENGL_GFX) ? false : true),
    use_gfx_headlock_((flags & GFX_HEADLOCK) ? true : false),
    gui_added_(false),
    dropped_frames_(0),
    num_frames_(0),
    prev_gfx_perf_log_(ApplicationClock::now()),
    prev_update_perf_log_(ApplicationClock::now()),
    graphics_lock_(graphics_mutex_, std::defer_lock),
    permissions_required_(permissions),
    permissions_granted_(false),
    permissions_requested_(false),
    owned_input_(false),
    input_handle_(ML_INVALID_HANDLE),
    head_handle_(ML_INVALID_HANDLE),
    controller_handle_(ML_INVALID_HANDLE),
    head_origin_set_(false) {
  ALOG_IF(ALOG_FATAL, !android_app_, "android_app may not be null");
  system_helper_ = std::make_unique<SystemHelper>(app);
  InitializeLifecycle();
  ReadCommandLineArgs();
  SetTitle(gflags::ProgramInvocationShortName());
  MLGraphicsFrameParamsExInit(&frame_params_);
  ml_input_handler_ = std::make_shared<ml::app_framework::MlInputHandler>();
  ml_keyboard_handler_ = std::make_shared<ml::app_framework::Keyboard>(ml_input_handler_);
}

Application::Application(struct android_app *app, int flags) : Application(app, {}, flags) {}

Application::~Application() {}

void Application::SetTitle(const char *title) {
  window_title_ = std::string(title);
  if (graphics_context_) {
    graphics_context_->SetTitle(window_title_.c_str());
  }
}

const std::string &Application::GetPackageName() const {
  return system_helper_->GetPackageName();
}

int Application::GetVersionCode() const {
  return system_helper_->GetVersionCode();
}

const std::string &Application::GetVersionName() const {
  return system_helper_->GetVersionName();
}

const std::string &Application::GetApplicationInstallDir() const {
  return system_helper_->GetApplicationInstallDir();
}

const std::string &Application::GetApplicationWritableDir() const {
  return system_helper_->GetApplicationWritableDir();
}

const std::string &Application::GetExternalFilesDir() const {
  return system_helper_->GetExternalFilesDir();
}

const std::string &Application::GetISO3Language() const {
  return system_helper_->GetISO3Language();
}

bool Application::IsInteractive() const {
  return system_helper_->IsInteractive();
}

void Application::KeepScreenOn([[maybe_unused]] bool on) const {
#ifdef ML_LUMIN
  //  As long as this window is visible to the user, keep the device's screen turned on and bright
  const uint32_t AWINDOW_FLAG_KEEP_SCREEN_ON = 0x00000080;
  if (on) {
    ANativeActivity_setWindowFlags(android_app_->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
  } else {
    ANativeActivity_setWindowFlags(android_app_->activity, 0, AWINDOW_FLAG_KEEP_SCREEN_ON);
  }
#endif
}

bool Application::CheckApplicationPermission(const std::string &permission_name) const {
  return system_helper_->CheckApplicationPermission(permission_name);
}

bool Application::CheckApplicationPermissions(const std::vector<std::string> &required_permissions,
                                              bool request_if_not_granted) {
  bool permissions_granted = true;
  std::vector<std::string> permissions_to_request;

  for (auto &permission : required_permissions) {
    ALOGI("Checking Permission %s", permission.c_str());
    if (!CheckApplicationPermission(permission)) {
      permissions_granted = false;
      if (request_if_not_granted) {
        ALOGI("Requesting Permission %s", permission.c_str());
        permissions_to_request.push_back(permission);
      }
    }
  }

  if (permissions_to_request.size() > 0) {
    RequestApplicationPermissions(permissions_to_request);
    permissions_requested_ = true;
  }

  return permissions_granted;
}

void Application::RequestApplicationPermission(const std::string &permission_name) const {
  system_helper_->RequestApplicationPermission(permission_name);
}

void Application::RequestApplicationPermissions(const std::vector<std::string> &permissions) const {
  system_helper_->RequestApplicationPermissions(permissions);
}

long Application::GetAvailableDiskBytes() const {
  return system_helper_->GetAvailableDiskBytes();
}

long Application::GetAvailableExternalBytes() const {
  return system_helper_->GetAvailableExternalBytes();
}

int Application::GetComputePackBatteryLevel() const {
  return system_helper_->GetComputePackBatteryLevel();
}

int Application::GetComputePackBatteryStatus() const {
  return system_helper_->GetComputePackBatteryStatus();
}

float Application::GetComputePackBatteryTemperature() const {
  return system_helper_->GetComputePackBatteryTemperature();
}

long Application::GetComputePackChargeTimeRemaining() const {
  return system_helper_->GetComputePackChargeTimeRemaining();
}

bool Application::IsControllerPresent() const {
  return system_helper_->IsControllerPresent();
}

int Application::GetControllerBatteryLevel() const {
  return system_helper_->GetControllerBatteryLevel();
}

int Application::GetControllerBatteryStatus() const {
  return system_helper_->GetControllerBatteryStatus();
}

bool Application::IsPowerBankPresent() const {
  return system_helper_->IsPowerBankPresent();
}

int Application::GetPowerBankBatteryLevel() const {
  return system_helper_->GetPowerBankBatteryLevel();
}

long Application::GetFreeMemory() const {
  return system_helper_->GetFreeMemory();
}

int Application::GetUnicodeChar(int event_type, int key_code, int meta_state) const {
  return system_helper_->GetUnicodeChar(event_type, key_code, meta_state);
}

int Application::GetLastTrimLevel() const {
  return system_helper_->GetLastTrimLevel();
}

long Application::GetTotalDiskBytes() const {
  return system_helper_->GetTotalDiskBytes();
}

long Application::GetTotalExternalBytes() const {
  return system_helper_->GetTotalExternalBytes();
}

bool Application::IsWifiOn() const {
  return system_helper_->IsWifiOn();
}

bool Application::IsNetworkConnected() const {
  return system_helper_->IsNetworkConnected();
}

bool Application::IsInternetAvailable() const {
  return system_helper_->IsInternetAvailable();
}

void Application::ReadCommandLineArgs() {
  std::unordered_map<std::string, std::string> &args = system_helper_->GetIntentExtras();

  if (!args.size()) return;

  command_line_args_.clear();

  for (const auto &key_val : args) {
    command_line_args_.push_back(key_val.first);
    if (!key_val.second.empty()) command_line_args_.push_back(key_val.second);
  }

  //  here's an example of a command to use to launch app with intent extras:
  //  adb shell am start -a android.intent.action.MAIN -e --KEY1 VALUE1 -e --KEY2 VALUE2 -n
  //  com.magicleap.capi.lifecycle/android.app.NativeActivity number of arguments shall be even for Gflags parsing to
  //  work properly otherwise lifecycle_arg_uri_ can be used directly

  if (command_line_args_.size() % 2 == 0) {
    std::vector<std::string> arg_vector = {GetPackageName()};
    for (const auto &arg_uri : command_line_args_) {
      std::vector<std::string> arg_uri_split = ml::app_framework::cli_args_parser::GetArgs(arg_uri.c_str());
      arg_vector.insert(arg_vector.end(), arg_uri_split.begin(), arg_uri_split.end());
    }
    cli_args_parser::ParseGflags(arg_vector);
  }
}

const std::vector<std::string> &Application::GetCommandLineArgs() const {
  return command_line_args_;
}

void Application::Initialize() {
  exit_signal_ = false;
  InitializePerception();
  // InitializeSignalHandler should be the last to be called
  InitializeSignalHandler();
}

void Application::InitializeLifecycle() {
  android_app_->userData = this;
  android_app_->onAppCmd = HandleAppCmdStatic;
  android_app_->onInputEvent = HandleInputEventStatic;
}

void Application::HandleAppCmdStatic(struct android_app *android_app, int32_t cmd) {
  auto app = reinterpret_cast<ml::app_framework::Application *>(android_app->userData);
  app->HandleAppCmd(cmd);
}

int32_t Application::HandleInputEventStatic(struct android_app *android_app, AInputEvent *event) {
  auto app = reinterpret_cast<ml::app_framework::Application *>(android_app->userData);
  const bool handled{app->HandleInputEvent(event)};
  return (handled ? 1 : 0);
}

void Application::InitializePerception() {
  MLPerceptionSettings perception_settings = {};
  UNWRAP_MLRESULT_FATAL(MLPerceptionInitSettings(&perception_settings));
  UNWRAP_MLRESULT_FATAL(MLPerceptionStartup(&perception_settings));
}

#if !ML_OSX
static void APIENTRY PrintGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                         const GLchar *message, const void *userParam) {
  (void)source;
  (void)type;
  (void)length;
  (void)userParam;
  int log_level = ALOG_ERROR;
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: log_level = ALOG_ERROR; break;
    case GL_DEBUG_SEVERITY_MEDIUM: log_level = ALOG_ERROR; break;
    case GL_DEBUG_SEVERITY_LOW: log_level = ALOG_WARN; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: log_level = ALOG_INFO; break;
    default: break;
  }
  ALOG(log_level, ALOG_TAG, "GL DEBUG(0x%X): %s", id, message);
}
#endif

void Application::InitializeGraphics() {
  // There can only be one MLGraphicsClient per process.
  // this mutex controls access to that.
  graphics_lock_.lock();

#ifdef ML_LUMIN
  graphics_context_.reset(new GraphicsContext());
#else
  graphics_context_.reset(
      new GraphicsContext(window_title_.c_str(), FLAGS_mirror_window, FLAGS_window_width, FLAGS_window_height));
#endif  // #ifdef ML_LUMIN
  graphics_context_->SetTitle(window_title_.c_str());
  graphics_context_->SetWindowCloseCallback([this]() {
    FinishActivity();
  });
  graphics_context_->MakeCurrent();

  if (!gladLoadGLLoader((GLADloadproc)ml::app_framework::GraphicsContext::GetProcAddress)) {
    ALOGA("gladLoadGLLoader() failed");
  }
#ifdef ML_LUMIN
  if (!gladLoadGLES2Loader((GLADloadproc)ml::app_framework::GraphicsContext::GetProcAddress)) {
    ALOGA("gladLoadGLES2Loader() failed");
  }
#endif

  auto *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
  auto *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
  auto *render = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
  ALOGI("OpenGL Version: %s", version);
  ALOGI("OpenGL Vendor: %s", vendor);
  ALOGI("OpenGL Render: %s", render);

#if !ML_OSX
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  // clang-format off
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, FLAGS_gl_debug_severity >= 1);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, FLAGS_gl_debug_severity >= 2);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, FLAGS_gl_debug_severity >= 3);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, FLAGS_gl_debug_severity >= 4);
  // clang-format on
  glDebugMessageCallback(PrintGlDebugMessage, nullptr);
#endif

  // Get ready to connect our GL context to the MLSDK graphics API
  MLGraphicsOptions graphics_options = {0, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float};
  if (use_gfx_headlock_) {
    graphics_options.graphics_flags |= MLGraphicsFlags_Headlock;
  }
  MLHandle opengl_context = graphics_context_->GetContextHandle();
  graphics_client_ = ML_INVALID_HANDLE;

  MLResult result = MLGraphicsCreateClientGL(&graphics_options, opengl_context, &graphics_client_);
  if (result != MLResult_Ok) {
    ALOGE("Failed to initialize graphics client %d.", result);
    FinishActivity();
    graphics_lock_.unlock();
    return;
  }

  // Init ml render targets
  MLGraphicsRenderTargetsInfo targets{};
  result = MLGraphicsGetRenderTargets(graphics_client_, &targets);
  if (result != MLResult_Ok) {
    ALOGE("Failed to get render targets %d", result);
    FinishActivity();
    graphics_lock_.unlock();
    return;
  }

  frame_params_.near_clip = targets.min_clip;

  for (int32_t i = 0; i < MLGraphics_BufferCount; ++i) {
    auto &buffer = targets.buffers[i];
    if (buffer.color.id == 0) {
      continue;
    }
    auto color_tex = std::make_shared<Texture>(GL_TEXTURE_2D_ARRAY, (GLuint)buffer.color.id, buffer.color.width,
                                               buffer.color.height, false);
    auto depth_tex = std::make_shared<Texture>(GL_TEXTURE_2D_ARRAY, (GLuint)buffer.depth.id, buffer.depth.width,
                                               buffer.depth.height, false);
    // Implicit assumption, left eye is always index 0
    auto left_render_target = std::make_shared<RenderTarget>(color_tex, depth_tex, 0, 0);
    auto right_render_target = std::make_shared<RenderTarget>(color_tex, depth_tex, 1, 1);

    // Only use color buffer as the key
    ml_render_target_cache_.insert(std::make_pair(std::make_pair(buffer.color.id, 0), left_render_target));
    ml_render_target_cache_.insert(std::make_pair(std::make_pair(buffer.color.id, 1), right_render_target));
  }

  // Initialize the new renderer and setting the post render camera callback
  renderer_.reset(new Renderer());
  auto post_camera_cb = [this](std::shared_ptr<CameraComponent> camera) {
    this->OnRenderCamera(camera);
  };
  auto pre_cb = [this]() {
    this->OnPreRender();
  };
  auto post_cb = [this]() {
    this->OnPostRender();
  };
  renderer_->SetPostRenderCameraCallback(post_camera_cb);
  renderer_->SetPreRenderCallback(pre_cb);
  renderer_->SetPostRenderCallback(post_cb);

  ml::IAssetManagerPtr asset_manager = ml::IAssetManager::Factory(android_app_);
  Registry::GetInstance()->Initialize(asset_manager);

  // Init nodes
  root_ = std::make_shared<Node>();
  for (unsigned int i = 0; i < camera_nodes_.size(); ++i) {
    camera_nodes_[i] = std::make_shared<Node>();
    auto camera = std::make_shared<CameraComponent>();
    camera_nodes_[i]->AddComponent(camera);
    root_->AddChild(camera_nodes_[i]);
  }

  // Setup light node
  light_node_ = std::make_shared<ml::app_framework::Node>();
  std::shared_ptr<LightComponent> light_component = std::make_shared<LightComponent>();
  light_component->SetLightStrength(5.0f);
  light_node_->AddComponent(light_component);
  root_->AddChild(light_node_);
}

void Application::InitializeSignalHandler() {
  auto signal_handler = [](int) {
    exit_signal_ = true;
  };

  if (SIG_ERR == std::signal(SIGINT, signal_handler)) {
    ALOGE("Failed to register the signal handler(SIGINT)");
  }
  if (SIG_ERR == std::signal(SIGTERM, signal_handler)) {
    ALOGE("Failed to register the signal handler(SIGTERM)");
  }
}

void Application::Terminate() {
  TerminatePerception();
  TerminateSignalHandler();
}

void Application::TerminatePerception() {
  MLResult ml_result = MLPerceptionShutdown();
  if (ml_result != MLResult_Ok) {
    ALOGE("MLPerceptionShutdown returned %d - %s", ml_result, MLGetResultString(ml_result));
  }
}

void Application::TerminateGraphics() {
  // Clean up the various graphics related
  // assets.
  while (!queue_.empty()) queue_.pop();
  ml_render_target_cache_.clear();
  root_.reset();
  light_node_.reset();
  renderer_.reset();

  // Shutdown MLGraphicsClient
  if (graphics_client_ != ML_INVALID_HANDLE) {
    MLResult ml_result = MLGraphicsDestroyClient(&graphics_client_);
    if (ml_result != MLResult_Ok) {
      ALOGE("MLGraphicsDestroyClient returned %d - %s", ml_result, MLGetResultString(ml_result));
    }
    graphics_client_ = ML_INVALID_HANDLE;
  }

  // And lastly close the OpenGL context.
  if (graphics_context_) {
    graphics_context_->UnMakeCurrent();
    graphics_context_.reset();
  }

  if (graphics_lock_.owns_lock()) {
    graphics_lock_.unlock();
  }
}

void Application::TerminateSignalHandler() {
  std::signal(SIGINT, SIG_DFL);
  std::signal(SIGTERM, SIG_DFL);
}

void Application::Update() {
  auto update_time = ApplicationClock::now();

  chrono::duration<float> delta_time = update_time - prev_update_time_;
  num_frames_++;
  auto delta = update_time - prev_update_perf_log_;
  if (FLAGS_perf_log_rate > 0 && delta >= chrono::duration<double>(FLAGS_perf_log_rate)) {
    auto fdelta = chrono::duration<float>(delta).count();
    ALOGD("%.4f s/frame (fps: %.4f)", fdelta / num_frames_, num_frames_ / fdelta);
    num_frames_ = 0;
    prev_update_perf_log_ += delta;
  }

  if (use_gfx_) {
    SetInitialHeadPose();
  }

  if (!UpdateNoPermissionGui()) {
    OnUpdate(delta_time.count());
  }
  prev_update_time_ = update_time;
}

void Application::SetInitialHeadPose() {
  if (!head_origin_set_ && MLHandleIsValid(head_handle_)) {
    MLSnapshot *snapshot = nullptr;
    MLResult res = MLPerceptionGetSnapshot(&snapshot);
    if (res != MLResult_Ok) {
      return;
    }
    MLTransform head_transform = {};
    res = MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform);
    if (res != MLResult_Ok) {
      return;
    }
    head_pose_origin_ = head_transform;
    MLPerceptionReleaseSnapshot(snapshot);
    head_origin_set_ = true;
    if (use_gui_) {
      const Pose gui_offset(glm::vec3(0.0f, 0.0f, -1.0f));
      GetGui().Place(head_pose_origin_.HorizontalRotationOnly() + gui_offset, false);
    }
  }
}

void Application::Render() {
  if (use_gfx_) {
    // Trivial BFS traversal, no partition
    VisitNode(root_);
    queue_.push(root_);
    while (!queue_.empty()) {
      auto current_node = queue_.front();
      auto children = current_node->GetChildren();
      for (auto child : children) {
        VisitNode(child);
        queue_.push(child);
      }
      queue_.pop();
    }

    MLGraphicsFrameInfo frame_info = {};
    MLGraphicsFrameInfoInit(&frame_info);
    MLResult out_result = MLGraphicsBeginFrameEx(graphics_client_, &frame_params_, &frame_info);
    if (MLResult_Pending == out_result || MLResult_Timeout == out_result) {
      ++dropped_frames_;
    } else if (MLResult_Ok == out_result) {
      MLHandle frame_handle = frame_info.handle;
      UpdateMLCamera(frame_info);
      Update();
      renderer_->Render();

      for (unsigned int i = 0; i < camera_nodes_.size(); ++i) {
        MLGraphicsSignalSyncObjectGL(graphics_client_, ml_sync_objs_[i]);
      }
      UNWRAP_MLRESULT(MLGraphicsEndFrame(graphics_client_, frame_handle));
      graphics_context_->SwapBuffers();

      auto now = chrono::steady_clock::now();
      if (FLAGS_perf_log_rate > 0 && now - prev_gfx_perf_log_ >= chrono::duration<double>(FLAGS_perf_log_rate)) {
        MLGraphicsClientPerformanceInfo perf_info = {};
        MLGraphicsGetClientPerformanceInfo(graphics_client_, &perf_info);
        constexpr float ns_per_s = 1e9f;

        ALOGD("framerate: %.4f fps", ns_per_s / perf_info.frame_start_cpu_frame_start_cpu_ns);
        ALOGD("frame_start_cpu_comp_acquire_cpu: %.4fs", perf_info.frame_start_cpu_comp_acquire_cpu_ns / ns_per_s);
        ALOGD("frame_start_cpu_frame_end_gpu: %.4fs", perf_info.frame_start_cpu_frame_end_gpu_ns / ns_per_s);
        ALOGD("frame_start_cpu_frame_start_cpu: %.4fs", perf_info.frame_start_cpu_frame_start_cpu_ns / ns_per_s);
        ALOGD("frame_duration_cpu: %.4fs", perf_info.frame_duration_cpu_ns / ns_per_s);
        ALOGD("frame_duration_gpu: %.4fs", perf_info.frame_duration_gpu_ns / ns_per_s);
        ALOGD("frame_internal_duration_cpu: %.4fs", perf_info.frame_internal_duration_cpu_ns / ns_per_s);
        ALOGD("frame_internal_duration_gpu: %.4fs", perf_info.frame_internal_duration_gpu_ns / ns_per_s);

        prev_gfx_perf_log_ = now;
      }
    } else {
      UNWRAP_MLRESULT(out_result);
    }
    renderer_->ClearQueues();
  } else {
    Update();
  }
}

void Application::VisitNode(std::shared_ptr<Node> node) {
  renderer_->Visit(node);
}

void Application::RunApp() {
  using namespace ml::app_framework;

  ALOGV("OnCreate");
  OnCreate(android_app_->savedState, android_app_->savedStateSize);
  Initialize();

  prev_update_time_ = ApplicationClock::now();
  // Loop waiting for events to process
  while (!android_app_->destroyRequested) {
    if (exit_signal_) {
      FinishActivity();
    }

#if !ML_LUMIN
    if (graphics_context_ && graphics_context_->GetDisplayHandle()) {
      if (glfwWindowShouldClose((GLFWwindow *)graphics_context_->GetDisplayHandle())) {
        FinishActivity();
      }
    }
#endif

    int events;
    struct android_poll_source *source;

    // If use_gfx_ is false, run the loop at roughly 60 loops per second.
    // If use_gfx_ is true, wait until event arrives if the app is not visible,
    //   otherwise grab only events already queued.
    int timeout = use_gfx_ ? -1 : 17;  // wait for event if visible or 60Hz loop.
    while (ALooper_pollAll(android_app_visible_ ? 0 : timeout, nullptr, &events, (void **)&source) >= 0) {
      if (source != nullptr) {
        source->process(android_app_, source);
      }

      if (android_app_->destroyRequested) {
        break;
      }
    }

    // Update() is called ony when the app is visible.
    // Render() is called when both app is visible and graphicsis used.
    if (android_app_visible_) {
      Render();
    }
  }

  Terminate();
  ALOGV("End loop. Total dropped frames: %zu", dropped_frames_);
}

void Application::InitGui() {
  if (!gui_added_) {
    CreateInternalController();
    CreateInternalInput();
    Gui &gui = Gui::GetInstance();
    gui.SetControllerHandle(controller_handle_).SetInputHandle(input_handle_).Initialize(GetGraphicsContext());
    GetRoot()->AddChild(GetGui().GetNode());
    if (head_origin_set_) {
      const Pose gui_offset(glm::vec3(0.0f, 0.0f, -1.0f));
      GetGui().Place(head_pose_origin_.HorizontalRotationOnly() + gui_offset, false);
    }
    gui_added_ = true;
  }
}

void Application::CleanupGui() {
  if (gui_added_) {
    GetGui().Hide();
    GetRoot()->RemoveChild(GetGui().GetNode());
    GetGui().Cleanup();
    gui_added_ = false;
    DestroyInternalHead();
    DestroyInternalController();
    DestroyInternalInput();
  }
}

void Application::CreateInternalHead() {
  UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_handle_));
  UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_handle_, &head_static_data_));
  SetInitialHeadPose();
}

void Application::DestroyInternalHead() {
  if (head_handle_ != ML_INVALID_HANDLE) {
    MLHeadTrackingDestroy(head_handle_);
    head_handle_ = ML_INVALID_HANDLE;
  }
}

void Application::CreateInternalController() {
  MLControllerConfiguration controller_config = {};
  controller_config.enable_fused6dof = false;
  UNWRAP_MLRESULT(MLControllerCreateEx(&controller_config, &controller_handle_));
}

void Application::DestroyInternalController() {
  if (controller_handle_ != ML_INVALID_HANDLE) {
    MLControllerDestroy(controller_handle_);
    controller_handle_ = ML_INVALID_HANDLE;
  }
}

void Application::CreateInternalInput() {
  if (!owned_input_) {
    MLInputCreate(&input_handle_);
    owned_input_ = true;
  }
}

void Application::DestroyInternalInput() {
  if (owned_input_) {
    MLInputDestroy(input_handle_);
    input_handle_ = ML_INVALID_HANDLE;
    owned_input_ = false;
  }
}

void Application::SetHeadHandle(MLHandle handle) {
  if (handle == ML_INVALID_HANDLE) {
    ALOGE("Cannot pass ML_INVALID_HANDLE");
  }
  DestroyInternalHead();
  head_handle_ = handle;
  UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_handle_, &head_static_data_));
}

void Application::SetControllerHandle(MLHandle handle) {
  if (handle == ML_INVALID_HANDLE) {
    ALOGE("Cannot pass ML_INVALID_HANDLE");
  }
  DestroyInternalController();
  controller_handle_ = handle;
  if (use_gui_) {
    GetGui().SetControllerHandle(controller_handle_);
  }
}

void Application::SetInputHandle(MLHandle handle) {
  if (handle == ML_INVALID_HANDLE) {
    ALOGE("Cannot pass ML_INVALID_HANDLE");
  }
  DestroyInternalInput();
  input_handle_ = handle;
  if (use_gui_) {
    GetGui().SetInputHandle(input_handle_);
  }
}

void Application::HandleAppCmd(const int32_t cmd) {
  switch (cmd) {
    case APP_CMD_START:
      ALOGI("APP_CMD_START");
      if (use_gfx_) {
        InitializeGraphics();
        if (use_gui_) {
          InitGui();
        }
        CreateInternalHead();
      }
      OnStart();
      break;
    case APP_CMD_RESUME:
      ALOGI("APP_CMD_RESUME");
      Resume();
      break;
    case APP_CMD_GAINED_FOCUS:
      ALOGI("APP_CMD_GAINED_FOCUS");
      FocusGained();
      break;
    case APP_CMD_LOST_FOCUS:
      ALOGI("APP_CMD_LOST_FOCUS");
      android_app_focussed_ = false;
      OnLostFocus();
      break;
    case APP_CMD_PAUSE:
      ALOGI("APP_CMD_PAUSE");
      OnPause();
      android_app_visible_ = false;
      break;
    case APP_CMD_STOP:
      ALOGI("APP_CMD_STOP");
      OnStop();
      if (use_gui_) {
        CleanupGui();
      }
      TerminateGraphics();
      break;
    case APP_CMD_SAVE_STATE: {
      ALOGI("APP_CMD_SAVE_STATE");
      android_app_->savedState = nullptr;
      android_app_->savedStateSize = 0;
      OnSaveState(android_app_->savedState, android_app_->savedStateSize);
      break;
    }
    case APP_CMD_DESTROY:
      ALOGI("APP_CMD_DESTROY");
      OnDestroy();
      break;

    // This doesn't fire in AR applications.
    case APP_CMD_INIT_WINDOW:
      ALOGI("APP_CMD_INIT_WINDOW");
      OnWindowInit();
      break;
      // This doesn't fire in AR applications.
    case APP_CMD_TERM_WINDOW:
      ALOGI("APP_CMD_TERM_WINDOW");
      OnWindowTerm();
      break;

    case APP_CMD_LOW_MEMORY:
      ALOGI("APP_CMD_LOW_MEMORY");
      OnLowMemory();
      break;
    case APP_CMD_INPUT_CHANGED:
      ALOGI("APP_CMD_INPUT_CHANGED");
      OnInputChanged();
      break;
    case APP_CMD_CONFIG_CHANGED:
      ALOGI("APP_CMD_CONFIG_CHANGED");
      OnConfigChanged();
      break;
    case APP_CMD_CONTENT_RECT_CHANGED:
    case APP_CMD_WINDOW_REDRAW_NEEDED:
    case APP_CMD_WINDOW_RESIZED: ALOGV("Ignored App Command %d", cmd); break;
    default: ALOGW("Unexpected Command Received %d", cmd); break;
  }
}

bool Application::HandleInputEvent(const AInputEvent *event) {
#if ML_LUMIN
  switch (AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_KEY:
      switch (AKeyEvent_getAction(event)) {
        case AKEY_EVENT_ACTION_DOWN: {
          int32_t key = AKeyEvent_getKeyCode(event);
          int32_t meta_state = AKeyEvent_getMetaState(event);
          uint32_t uni_value = GetUnicodeChar(AKEY_EVENT_ACTION_DOWN, key, meta_state);
          ALOGI("Key Down Event: %x", key);
          ml_input_handler_->OnKeyDown(key, meta_state);
          // Handling only character unicode values
          // To-Do there needs to be a better way to handle character only unicode values.
          // Refer: https://unicode-table.com/en/
          if (uni_value >= 0x0020) {
            ALOGI("Key Char unicode(hex): %x", uni_value);
            ml_input_handler_->OnChar(uni_value);
          }
          break;
        }
        case AKEY_EVENT_ACTION_UP: {
          int32_t key = AKeyEvent_getKeyCode(event);
          int32_t meta_state = AKeyEvent_getMetaState(event);
          ml_input_handler_->OnKeyUp(key, meta_state);
          ALOGI("Key Up Event");
          break;
        }
      }
  }
#else
  (void)event;
#endif
  return true;  // return true if the event was handled.
}

void Application::FinishActivity() {
  ANativeActivity_finish(android_app_->activity);
}

void Application::UpdateMLCamera(const MLGraphicsFrameInfo &frame_info) {
  using namespace ml::app_framework;

  predicted_display_time = frame_info.predicted_display_time;

  for (uint32_t camera_index = 0; camera_index < frame_info.num_virtual_cameras; ++camera_index) {
    std::shared_ptr<CameraComponent> cam = camera_nodes_[camera_index]->GetComponent<CameraComponent>();

    const MLRectf &viewport = frame_info.viewport;
    cam->SetRenderTarget(ml_render_target_cache_[std::make_pair(frame_info.color_id, camera_index)]);

#ifndef ML_LUMIN
    if (FLAGS_mirror_window && camera_index == 1) {
      // add a blit target, only for one eye
      auto dims = graphics_context_->GetFramebufferDimensions();
      if (!default_render_target_) {
        default_render_target_ = std::make_shared<RenderTarget>(dims.first, dims.second);
      }
      default_render_target_->SetWidth(dims.first);
      default_render_target_->SetHeight(dims.second);
      cam->SetBlitTarget(default_render_target_);
    }
#endif  // #ifndef ML_LUMIN

    cam->SetViewport(glm::vec4((GLint)viewport.x, (GLint)viewport.y, (GLsizei)viewport.w, (GLsizei)viewport.h));

    auto &current_camera = frame_info.virtual_cameras[camera_index];
    auto proj = glm::make_mat4(current_camera.projection.matrix_colmajor);
    cam->SetProjectionMatrix(proj);
    camera_nodes_[camera_index]->SetWorldRotation(glm::make_quat(current_camera.transform.rotation.values));
    camera_nodes_[camera_index]->SetWorldTranslation(glm::make_vec3(current_camera.transform.position.values));
    ml_sync_objs_[camera_index] = current_camera.sync_object;

    if (camera_index == 1) {
      // Light will follow one eye
      light_node_->SetWorldTranslation(glm::make_vec3(current_camera.transform.position.values));
    }
  }
}

void Application::Resume() {
  android_app_visible_ = true;
  // OnResume check for permission, but do not request permission
  // If needed, permissions should be requested after gained focus
  if (!ArePermissionsGranted()) {
    CheckPermissionAndPerformActions(false);
  }
  OnResume();
}

void Application::FocusGained() {
  android_app_focussed_ = true;
  OnGainedFocus();
  // If the permissions were not granted already, request for permission
  if (!ArePermissionsGranted()) {
    CheckPermissionAndPerformActions(true);
  }
}

void Application::UpdateFrameParams(const MLGraphicsFrameParamsEx &frame_params) {
  frame_params_ = frame_params;
}

Gui &Application::GetGui() {
  return Gui::GetInstance();
}

bool Application::ArePermissionsGranted() const {
  return (permissions_granted_ || permissions_required_.empty());
}

bool Application::UpdateNoPermissionGui() {
  if (permissions_granted_ || permissions_required_.empty()) {
    return false;
  }
  if (use_gui_) {
    RenderNoPermissionGui();
  }
  return true;
}

void Application::CheckPermissionAndPerformActions(bool request_if_not_granted) {
  const bool was_requested = permissions_requested_;
  permissions_granted_ = CheckApplicationPermissions(permissions_required_, request_if_not_granted);

  if (was_requested && !permissions_granted_) {
    if (use_gui_) {
      // Perms requested and not granted -> showing no-perms gui
      GetGui().Show();
    } else {
      // Perms requested and not granted -> logging msg and exitting the app
      LogNoPermissions();
      FinishActivity();
    }
  }

  if (use_gui_ && permissions_granted_) {
    // Perms granted -> hiding gui
    GetGui().Hide();
  }
}

void Application::LogNoPermissions() {
  ALOGE("This application needs the following permission(s) and will not work until these are granted:");
  for (const auto &permission : permissions_required_) {
    if (!CheckApplicationPermission(permission)) {
      ALOGE("%s", permission.c_str());
    }
  }
}

void Application::RenderNoPermissionGui() {
  auto &gui = GetGui();
  gui.BeginUpdate();
  bool is_running = true;
  if (gui.BeginDialog("Permissions Request", &is_running,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_NoCollapse)) {
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "This application needs the following permission(s)\nand will not "
                                                   "work until these are granted:");
    for (const auto &permission : permissions_required_) {
      if (!CheckApplicationPermission(permission)) {
        ImGui::BulletText("%s", permission.c_str());
      }
    }
    ImGui::NewLine();
    ImGui::NewLine();
    static float button_width = 100.0f;  ///> The 100.0f is just a guess size for the first frame
    const float pos = button_width + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SameLine((ImGui::GetWindowWidth() - pos) / 2.f);  ///> Center horizontally the next item
    if (ImGui::Button("Re-request permissions")) {
      RequestApplicationPermissions(permissions_required_);
    }
    button_width = ImGui::GetItemRectSize().x;
    ImGui::NewLine();
    ImGui::TextDisabled("Note: If You have chosen \"Deny and don't ask again\",\n\tYou can exit the app by closing "
                        "this window.");
  }
  gui.EndDialog();
  gui.EndUpdate();
  if (!is_running) {
    FinishActivity();
  }
}

std::optional<Pose> Application::GetHeadPoseOrigin() const {
  if (head_origin_set_) {
    return head_pose_origin_;
  }
  return {};
}

}  // namespace app_framework
}  // namespace ml
