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

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>

#include <app_framework/components/camera_component.h>
#include <app_framework/graphics_context.h>
#include <app_framework/input/keyboard.h>
#include <app_framework/input/ml_input_handler.h>
#include <app_framework/node.h>
#include <app_framework/system_helper.h>

#include <android_native_app_glue.h>
#include <ml_graphics.h>
#include <ml_head_tracking.h>

using ApplicationClock = std::chrono::steady_clock;

namespace std {
template <>
struct hash<std::pair<MLHandle, uint32_t>> {
  size_t operator()(const std::pair<MLHandle, uint32_t> &key) const {
    return key.first << 32 | key.second;
  }
};
};  // namespace std

namespace ml {
namespace app_framework {

class Renderer;
class Gui;

/*! \brief The base class for sample applications.

    Application class to ease the development of Augmented Reality
    apps for Magic Leap Hardware.

  \code{.cpp}
  class GfxTemplateApp : public ml::app_framework::Application {
    public:
      GfxTemplateApp(struct android_app* state) : ml::app_framework::Application(state, USE_GUI) {}
  }

  void android_main(struct android_app* state) {
    GfxTemplateApp app(state);
    app.RunApp();
  }
  \endcode
*/
class Application {
public:
  /*! Options that can be passed while initiating the application class.

    Example:
    \code{.cpp}
    (USE_GUI | GFX_HEADLOCK)
    \endcode
 */
  typedef enum ApplicationFlags {
    NONE = 0,               /**< Specify no options */
    USE_GUI = 1 << 1,       /**< to automatically initiate and cleanup Gui
                             * resources node. The Gui node is added to the
                             * rootnode. */
    NO_OPENGL_GFX = 1 << 2, /**< to disable the OpenGL graphics setup. The child
                             * class is expected to handle all graphics related
                             * operations if this is used. */
    GFX_HEADLOCK = 1 << 3,  /**< to start the graphics subsystem in headlocked
                             * mode. */
    ApplicationFlags_Ensure32Bits = 0x7FFFFFFF
  } ApplicationFlags;

  /*!
    \brief Constructor. Uses gflags to parse the arguments.
    \param app is the application state passed into the
              application from android_native_app_glue.
    \param pass application option flags eg: (USE_GUI | GFX_HEADLOCK).
  */
  Application(android_app *app, int flags = USE_GUI);

  /*!
    \brief Constructor. Uses gflags to parse the arguments.
    \param app is the application state passed into the
              application from android_native_app_glue.
    \param permissions list of permissions to be requested and checked
              while using ArePermissionsGranted() and UpdateNoPermissionGui() methods.
    \param pass application option flags eg: (USE_GUI | GFX_HEADLOCK).
  */
  Application(android_app *app, std::vector<std::string> permissions, int flags = USE_GUI);

  /*! Destructor */
  virtual ~Application();

  /*!
    \brief Runs the application.
  */
  void RunApp();

  /*!
    \brief Sets the title of the GLWindow on host.
    \param title The UTF-8 encoded window title.
  */
  void SetTitle(const char *title);

  std::shared_ptr<Node> GetRoot() const {
    return root_;
  }

  std::shared_ptr<Node> GetDefaultLightNode() const {
    return light_node_;
  }

  /*!
    \brief Update the graphics frame params.

    This includes near and far clip. Take a look at ml_graphics.h
    MLGraphicsFrameParamsExInit function which has defaults for the struct members.
  */
  void UpdateFrameParams(const MLGraphicsFrameParamsEx &frame_params);

  MLGraphicsFrameParamsEx GetFrameParams() const {
    return frame_params_;
  };

  std::shared_ptr<ml::app_framework::MlInputHandler> GetInputHandler() const {
    return ml_input_handler_;
  }

  std::shared_ptr<ml::app_framework::Keyboard> GetKeyboardHandler() const {
    return ml_keyboard_handler_;
  }

  /*! \brief Returns true if the application is currently visible.

    Note that this doesn't necessarily mean that the application
    has focus.
  */
  bool IsVisible() const {
    return android_app_visible_;
  }

  /*! \brief Returns true if the application has the input focus.
   */
  bool IsFocussed() const {
    return android_app_focussed_;
  }

  MLTime GetPredictedDisplayTime() const {
    return predicted_display_time;
  }

protected:
  /*! \name JNI Convenience access functions.

    Functions in this section will access the JVM. Note that these
    function may only be called from the android_main thread, this
    includes all the Life Cycle and Visiblity events.
  */
  ///@{
  /*! Returns the command line arguments given to the activity
      via Intent Bundles or via ADB.

      adb .... --es "Message" "hello!"
  */
  const std::vector<std::string> &GetCommandLineArgs() const;

  /*! Returns the version code as set in the AndroidManifest.xml.

      https://developer.android.com/studio/publish/versioning
  */
  int GetVersionCode() const;

  /*! Returns the version name as set in the AndroidManifest.xml.

      https://developer.android.com/studio/publish/versioning
  */
  const std::string &GetVersionName() const;

  /*! Returns the package name of the Application.
   */
  const std::string &GetPackageName() const;

  /*! Returns the full path to the base APK for this application.
   */
  const std::string &GetApplicationInstallDir() const;

  /*! Returns the full path to the default directory assigned to the
      package for its persistent data.
  */
  const std::string &GetApplicationWritableDir() const;

  /*! Returns the absolute path to the directory on the primary
      shared/external storage device where the application can place persistent
      files it owns. These files are internal to the applications, and not
      typically visible to the user as media.

      https://developer.android.com/reference/android/content/Context
  */
  const std::string &GetExternalFilesDir() const;

  /*! Returns a three-letter abbreviation of current locale's language.
   */
  const std::string &GetISO3Language() const;

  /*! Retuns true if the screen is on and false if the screen is off.
   */
  bool IsInteractive() const;

  /*! Keeps screen on and bright as long as application is visible to the user.
   *  Not supported for host apps.
   */
  void KeepScreenOn(bool on = true) const;

  /*! Checks if the application has the given permission granted.

      https://developer.android.com/training/permissions/requesting
  */
  bool CheckApplicationPermission(const std::string &permission_name) const;

  /*! \brief CheckApplicationPermissions returns true if all permissions have been granted.

      If the user has not yet granted all permissions, this function always returns false, even
      if request_if_not_granted is true. This is because requesting permissions is an
      asynchronous operation. This means that this function finds the permissions in the
      required_permissions list that have not yet been granted and will request access
      for these and return false.

      Requesting access is asynchronous and will present a UX element to the
      user if USE_GUI is used else a message is printed to the LOG and the application exits.

      The call effectively pushes the current application to the background or
      at minimum causes it to lose focus.  The function however will
      immediately return false.

      \param required_permissions, the vector of permissions to check.
      \param request_if_not_granted if set to true the function will request all
                                 permissions in the required_permission_list_ variable.

     \returns true if all permissions were granted.
  */
  bool CheckApplicationPermissions(const std::vector<std::string> &required_permissions, bool request_if_not_granted);

  /*! Request the given permission.

      https://developer.android.com/training/permissions/requesting
  */
  void RequestApplicationPermission(const std::string &permission_name) const;

  /*! Request the given permissions.

      https://developer.android.com/training/permissions/requesting
  */
  void RequestApplicationPermissions(const std::vector<std::string> &permissions) const;

  /*! Returns the available bytes of the internal disk root directory.

      https://developer.android.com/reference/android/os/Environment#getRootDirectory()
      https://developer.android.com/reference/android/os/StatFs#getAvailableBytes()
  */
  long GetAvailableDiskBytes() const;

  /*! Returns the available bytes of the external disk root directory.

     https://developer.android.com/reference/android/os/Environment#getExternalStorageDirectory()
     https://developer.android.com/reference/android/os/StatFs#getAvailableBytes()
  */
  long GetAvailableExternalBytes() const;

  /*! Returns the battery level in % of the battery remaining of the device only.(not controller)

      https://developer.android.com/reference/android/os/BatteryManager#BATTERY_PROPERTY_CAPACITY
  */
  int GetComputePackBatteryLevel() const;

  /*! Returns the battery status, charging/discharging etc. of the device only.

      https://developer.android.com/reference/android/os/BatteryManager#BATTERY_PROPERTY_STATUS
  */
  int GetComputePackBatteryStatus() const;

  /*! Returns the temperature of the compute pack battery in degrees Celsius

      https://developer.android.com/reference/android/os/BatteryManager#EXTRA_TEMPERATURE
   */
  float GetComputePackBatteryTemperature() const;

  /*! Returns the remaining charge time in ms when device battery is plugged in and charging.

      https://developer.android.com/reference/android/os/BatteryManager#computeChargeTimeRemaining()
  */
  long GetComputePackChargeTimeRemaining() const;

  /*! Checks if the controller is present. */
  bool IsControllerPresent() const;

  /*! Returns the level in % of the controller battery. */
  int GetControllerBatteryLevel() const;

  /*! Returns the battery status, charging/discharging etc of the controller.
    Unknown = 1,
    Charging = 2,
    Discharging = 3,
    NotCharging = 4,
    Full = 5
  */
  int GetControllerBatteryStatus() const;

  /*! Returns the power bank's present status */
  bool IsPowerBankPresent() const;

  /*! Returns the battery level in % of the power bank. */
  int GetPowerBankBatteryLevel() const;

  /*! Returns the amount of free memory available in bytes. */
  long GetFreeMemory() const;

  /*! Returns the unicode character generated by the specified key and meta key state combination.

      https://developer.android.com/reference/android/view/KeyEvent#getUnicodeChar(int)
  */
  int GetUnicodeChar(int event_type, int key_code, int meta_state) const;

  /*! Returns the appications last trim memory state.

    https://developer.android.com/reference/android/content/ComponentCallbacks2#TRIM_MEMORY_RUNNING_LOW
  */
  int GetLastTrimLevel() const;

  /*! Returns the total bytes of the internal disk root directory.

     https://developer.android.com/reference/android/os/Environment#getRootDirectory()
     https://developer.android.com/reference/android/os/StatFs#getTotalBytes()
   */
  long GetTotalDiskBytes() const;

  /*! Returns the total bytes of the external disk root directory.

     https://developer.android.com/reference/android/os/Environment#getExternalStorageDirectory()
     https://developer.android.com/reference/android/os/StatFs#getTotalBytes()
  */
  long GetTotalExternalBytes() const;

  /*! Checks if device Wifi is turned ON

     https://developer.android.com/reference/android/net/wifi/WifiManager
  */
  bool IsWifiOn() const;

  /*! Checks if device is connected to any network

      https://developer.android.com/reference/android/net/ConnectivityManager#getNetworkCapabilities(android.net.Network)
      https://developer.android.com/reference/android/net/Network
  */
  bool IsNetworkConnected() const;

  /*! Checks if device is connected to any network and internet is available

      https://developer.android.com/reference/android/net/ConnectivityManager#getNetworkCapabilities(android.net.Network)
      https://developer.android.com/reference/android/net/Network
  */
  bool IsInternetAvailable() const;
  ///@}

  /*! Sets an external head handle if a valid handle is sent else creates one internally.
   */
  void SetHeadHandle(MLHandle handle);

  /*! Sets an external controller handle if a valid handle is sent else creates one internally.
   */
  void SetControllerHandle(MLHandle handle);

  /*! Sets an external input handle if a valid handle is sent else creates one internally.
   */
  void SetInputHandle(MLHandle handle);

  /*! \name Activity Life Cycle Events.

      \brief ml::app_framework::Application follows the Android Activity Life cycle.

      More details on the life cycle of applications can be found here:
      https://developer.android.com/guide/components/activities/activity-lifecycle

      Typical flow is as follows:

      1. [Activity is Launched]
      2. <OnCreate>
      3. <OnStart>
      4. <OnResume>

      5. [Activity Is Running]

      6. <OnPause>
      7. <OnStop>
      8. <OnDestroy>
      9. [Activity is Destroyed]

      Do review the full lifecycle documentation, as it will clarify the potential
      state changes that can occur.

      For Visibility and Focus events see the #OnWindowInit and #OnGainedFocus functions.
  */
  ///@{

  /*! \brief OnCreate is called at the start of the main thread.

    This method is called before OnStart or any of the other lifecycle events. It allows
    the developer to restore previous state if it was available. See #OnSaveState to learn how to
    store the state.

    \param saved_state pointer to the data stored in #OnSaveState.
    \param saved_state_size the length of the memory that saved_state is pointing to.

    \code{.cpp}
    // Purely an example Application state structure.
    typedef struct {
      int counter;
      bool flag;
    } MyState;
    // MyState is held on MyApp object under the name app_state_;

    MyApp::OnCreate(const void *saved_state, size_t saved_state_size) {
      if (saved_state != nullptr && saved_state_size == sizeof(app_state_) ) {
        // load saved app_state_.
        memcpy(&app_state_, saved_state, saved_state_size);
      } else {
        // initialize app_state_.
        app_state_.counter = 0;
        app_state_.flag = false;
      }
    }
    \endcode
  */
  virtual void OnCreate(const void *saved_state, size_t saved_state_size) {
    (void)saved_state;
    (void)saved_state_size;
  }
  /*!
    \brief Activity is starting up, during OnStart no graphics are available yet.
  */
  virtual void OnStart() {}
  /*!
    \brief Activity is about to become visible to user, graphics not yet ready. */
  virtual void OnResume() {}
  /*!
    \brief The activity has gone to the background and is no longer visible. */
  virtual void OnPause() {}
  /*!
    \brief The activity is being stopped completely. */
  virtual void OnStop() {}
  /*!
    \brief The activity is being destroyed. */
  virtual void OnDestroy() {}
  /*!
    \brief This is called when the overall system is running low on memory, and
    actively running processes should trim their memory usage. */
  virtual void OnLowMemory() {}

  /*!
    \brief Called before shutdown giving the activity a chance to save state.

    \param data a reference to a pointer for holding the state.
    \param size the length of the memory that data is pointing to.

    Use malloc to allocate the state data. The underlying code will
    take ownership of the allocated data and free it on the next #OnResume call.
    See #android_app::savedState for more info.

    \code{.cpp}
    // Purely an example Application state structure.
    typedef struct {
      int counter;
      bool flag;
    } MyState;
    // MyState is held on MyApp object under the name app_state_;

    MyApp::OnSaveState(void *&data, size_t &size) {
      size = sizeof(app_state_);
      data = malloc(size);
      memcpy(data, &app_state_, size);
    }
    \endcode

    \ref https://developer.android.com/reference/games/game-activity/structandroid/app#savedstate
    */
  virtual void OnSaveState(void *&data, size_t &size) {
    data = nullptr;
    size = 0;
  }

  /*! \brief Configuration of the device has changed.
   */
  void OnConfigChanged(){};

  /*! \brief The input queue has changed.
   */
  void OnInputChanged(){};

  /*!
    Finishes the activity
  */
  void FinishActivity();
  ///@}

  /*!
    \name Visibility and Focus Events

    \brief Beside the lifecycle events ml::app_framework::Application provides visible and focus events.

    The functions below can be overridden to receive notifications of these events. It is best to rely on these
    for visibility and focus instead of #OnStart / #OnResume.

    The graphics stack will be started before the OnWindowInit event is fired.
  */
  ///@{
  /*!
    Graphics are now ready to be used, IsVisible() => true. */
  virtual void OnWindowInit() {}
  /*!
    Graphics are about to be shutdown, clean up anything graphics related and stop drawing. IsVisible() => false. */
  virtual void OnWindowTerm() {}
  /*!
    \brief Focus was gained.

    The activity has the user focus. IsFocussed() => true */
  virtual void OnGainedFocus() {}
  /*!
    \brief Focus was lost.

    The activity might still be visible but has lost user focus, an example is when a
    system dialog pops over the application. IsFocussed() => false */
  virtual void OnLostFocus() {}

  /*!
    \brief Pre-render state update callback.

    Render loop callback when the application is visible. */
  virtual void OnUpdate(float delta_time_scale) {
    (void)delta_time_scale;
  }

  /*!
    \brief Post-render camera state callback.

    Called after a frame is rendered for each camera */
  virtual void OnRenderCamera(std::shared_ptr<CameraComponent> cam) {
    (void)cam;
  }

  /*!
    \brief Pre-render state callback.

    Called before a frame is rendered */
  virtual void OnPreRender() {}

  /*!
    \brief Post-render state callback.

    Called after a frame is rendered */
  virtual void OnPostRender() {}
  ///@}

  /*!
    Returns the graphics context. */
  GraphicsContext *GetGraphicsContext() {
    return graphics_context_.get();
  }

  /*!
    Returns the graphics client. */
  MLHandle GetGraphicsClient() {
    return graphics_client_;
  }

  /*! Returns a #Gui object.

    Hidden by default (see #Gui::Show).

    Note that this is a #Gui resource and there can only be one #Gui object in
    your application. The second #Gui resource will terminate your app with an
    appropriate log message.
   */
  Gui &GetGui();

  /*!
    Checks permissions based on the list given in constructor.
  */
  bool ArePermissionsGranted() const;

  /*! Returns the first available headpose since the app startup.

    If headpose was not yet available, returned optional will be empty.
   */
  std::optional<Pose> GetHeadPoseOrigin() const;

private:
  void ReadCommandLineArgs(); /*!< Reads activity intent extras key value pairs consumed if available */

  /**
   * Handle and dispatch an app cmd event from android_app.
   *
   * @param cmd The command, from the APP_CMD enum in android_native_app_glue
   */
  void HandleAppCmd(int32_t cmd);

  /**
   * Handle and dispatch an android input event from android_app.
   *
   * @param event The android input event.
   * @return True if the event was handled by this application, false otherwise.
   */
  bool HandleInputEvent(const AInputEvent *event);

  // Static exports for Android Native Glue.
  static void HandleAppCmdStatic(struct android_app *android_app, int32_t cmd);
  static int32_t HandleInputEventStatic(struct android_app *android_app, AInputEvent *event);

  void Initialize();
  void InitializeLifecycle();
  void InitializeLifecycleSelfInfo();
  void InitializePerception();
  void InitializeGraphics();
  void InitializeSignalHandler();
  void SignalHandler(int signal);

  void Terminate();
  void TerminatePerception();
  void TerminateGraphics();
  void TerminateSignalHandler();

  void Update();
  void Render();

  void CheckPermissionAndPerformActions(bool request_if_not_granted);
  void LogNoPermissions();
  void RenderNoPermissionGui();
  bool UpdateNoPermissionGui();
  void InitGui();
  void CleanupGui();

  // Create and destroy methods internal head, controller and inputs
  void CreateInternalHead();
  void SetInitialHeadPose();
  void DestroyInternalHead();
  void CreateInternalController();
  void DestroyInternalController();
  void CreateInternalInput();
  void DestroyInternalInput();

  void Resume();
  void FocusGained();

  // Android android_native_app_glue pointer.
  struct android_app *android_app_;
  bool android_app_visible_;
  bool android_app_focussed_;

  std::shared_ptr<ml::app_framework::MlInputHandler> ml_input_handler_;
  std::shared_ptr<ml::app_framework::Keyboard> ml_keyboard_handler_;

  // App Info
  ApplicationClock::time_point prev_update_time_;
  const bool use_gui_;
  const bool use_gfx_;
  const bool use_gfx_headlock_;
  bool gui_added_;
  static std::atomic<bool> exit_signal_;
  std::string window_title_;

  // Graphics
  std::unique_ptr<GraphicsContext> graphics_context_;
  MLHandle graphics_client_;
  MLGraphicsFrameParamsEx frame_params_;
  // number of frames dropped.
  size_t dropped_frames_;
  // Total number of frames rendered.
  uint32_t num_frames_;

  // command line arguments
  std::vector<std::string> command_line_args_;

  void UpdateMLCamera(const MLGraphicsFrameInfo &frame_info);
  std::array<MLHandle, 2> ml_sync_objs_;
  // Texture handle, and layer index as the key
  std::unordered_map<std::pair<MLHandle, uint32_t>, std::shared_ptr<IRenderTarget>> ml_render_target_cache_;
  std::shared_ptr<IRenderTarget> default_render_target_;

  // Renderer
  std::unique_ptr<app_framework::Renderer> renderer_;

  // Node
  std::shared_ptr<Node> light_node_;
  std::shared_ptr<Node> root_;
  std::queue<std::shared_ptr<Node>> queue_;
  void VisitNode(std::shared_ptr<Node> node);

  // Default render camera
  std::array<std::shared_ptr<Node>, 2> camera_nodes_;

  // this is for periodically logging graphics performance info
  ApplicationClock::time_point prev_gfx_perf_log_;

  // this only logs the framerate, but it's useful for both graphical and non-graphical apps
  ApplicationClock::time_point prev_update_perf_log_;

  // static variables
  static const std::string kEmptyStr;

  std::unique_ptr<SystemHelper> system_helper_;

  static std::mutex graphics_mutex_;
  std::unique_lock<std::mutex> graphics_lock_;

  // Permissions helper stuff
  std::vector<std::string> permissions_required_;
  bool permissions_granted_;
  bool permissions_requested_;

  bool owned_input_;

  MLHandle input_handle_;
  MLHandle head_handle_;
  MLHandle controller_handle_;
  MLHeadTrackingStaticData head_static_data_ = {};
  Pose head_pose_origin_;
  bool head_origin_set_;

  MLTime predicted_display_time{0};
};

}  // namespace app_framework
}  // namespace ml
