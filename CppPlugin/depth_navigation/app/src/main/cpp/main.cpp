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

#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "confidence_material.h"
#include "depth_material.h"
#include "marker_detection.h"
#include "marker_overlay_material.h"

#include <time.h>
#include <cstdlib>
#include <future>

// OpenCV headers
#include <opencv2/opencv.hpp>

#include <app_framework/application.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/gui.h>
#include <app_framework/material/textured_material.h>
#include <app_framework/toolset.h>

#include <ml_depth_camera.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>
#include <ml_time.h>

#ifdef ML_LUMIN
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#endif

using namespace ml::app_framework;
using namespace std::chrono_literals;

namespace {
  float GetScale(int num_rects) {
    const float grid_size = ceil(sqrtf(num_rects * 1.f));
    const float scale_factor = 1.25f;
    return scale_factor / grid_size;
  }

  std::vector<glm::vec3> GetTranslationGrid(int num_rects) {
    const int grid_size = static_cast<int>(ceil(sqrtf(num_rects * 1.f)));
    const float step_size = GetScale(num_rects);
    const float z = -2.5f;
    std::vector<glm::vec3> res;
    res.reserve(grid_size * grid_size);
    int counter = 0;
    for (int y = grid_size - 1; y >= 0; --y) {
      for (int x = 0; x < grid_size; ++x) {
        const float x_pos = x * step_size - 0.25f;
        const float y_pos = y * step_size - 0.30f;
        res.emplace_back(x_pos, y_pos, z);
        if (++counter == num_rects) return res;
      }
    }
    return res;
  }

  const char* GetMLDepthCameraFrameTypeString(const MLDepthCameraFrameType& frame_type) {
    switch (frame_type) {
      case MLDepthCameraFrameType_LongRange: return "LongRange";
      case MLDepthCameraFrameType_ShortRange: return "ShortRange";
      default: return "Error";
    }
  }

  const char* GetMLDepthCameraStreamString(const MLDepthCameraStream& stream) {
    switch (stream) {
      case MLDepthCameraStream_LongRange: return "LongRange";
      case MLDepthCameraStream_ShortRange: return "ShortRange";
      default: return "Error";
    }
  }

  const char* GetMLDepthCameraFlagsString(const MLDepthCameraFlags& flag) {
    switch (flag) {
      case MLDepthCameraFlags_DepthImage: return "DepthImage";
      case MLDepthCameraFlags_Confidence: return "Confidence";
      case MLDepthCameraFlags_AmbientRawDepthImage: return "AmbientRawDepthImage";
      case MLDepthCameraFlags_RawDepthImage: return "RawDepthImage";
      default: return "Error";
    }
  }

  const char* GetMLDepthCameraFrameRateString(const MLDepthCameraFrameRate& flag) {
    switch (flag) {
      case MLDepthCameraFrameRate_1FPS: return "1 Hz";
      case MLDepthCameraFrameRate_5FPS: return "5 Hz";
      case MLDepthCameraFrameRate_25FPS: return "25 Hz";
      case MLDepthCameraFrameRate_30FPS: return "30 Hz";
      case MLDepthCameraFrameRate_50FPS: return "50 Hz";
      case MLDepthCameraFrameRate_60FPS: return "60 Hz";
      default: return "Error";
    }
  }
}

class DepthCameraApp : public Application {
public:
  DepthCameraApp(struct android_app *state)
    : Application(state, std::vector<std::string>{"com.magicleap.permission.DEPTH_CAMERA"}, USE_GUI),
    cam_context_(ML_INVALID_HANDLE),
    last_dcam_frametype_(MLDepthCameraFrameType_Ensure32Bits),
    last_dcam_intrinsics_{},
    last_dcam_frame_number_(-1),
    last_dcam_pose_{},
    is_preview_invalid_(false),
    cap_idx_(0) {

    // Fill arrays with initial values.
    min_values_.fill(0);
    max_values_.fill(0);
    texture_id_.fill(0);
    texture_width_.fill(544);
    texture_height_.fill(480);
    blank_depth_tex_.resize(texture_width_.front() * texture_height_.front());

    // Clean timestamp char array.
    memset(last_dcam_timestamp_str_, 0, sizeof(last_dcam_timestamp_str_));

    // Initialize limit values for shaders. These values are BY NO MEANS suggested ranges for depth camera data.
    min_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage)] = 0.0f;
    max_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage)] = 5.0f;
    distance_limit_[GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage)] = 7.5f;
    legend_unit_[GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage)] = 'm';

    min_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_Confidence)] = 0.0f;
    max_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_Confidence)] = 100.0f;
    distance_limit_[GetIndexFromCameraFlag(MLDepthCameraFlags_Confidence)] = 100.0f;
    legend_unit_[GetIndexFromCameraFlag(MLDepthCameraFlags_Confidence)] = '%';

    min_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_AmbientRawDepthImage)] = 5.0f;
    max_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_AmbientRawDepthImage)] = 2000.0f;
    distance_limit_[GetIndexFromCameraFlag(MLDepthCameraFlags_AmbientRawDepthImage)] = 2000.0f;
    legend_unit_[GetIndexFromCameraFlag(MLDepthCameraFlags_AmbientRawDepthImage)] = ' ';

    min_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_RawDepthImage)] = 5.0f;
    max_distance_[GetIndexFromCameraFlag(MLDepthCameraFlags_RawDepthImage)] = 2000.0f;
    distance_limit_[GetIndexFromCameraFlag(MLDepthCameraFlags_RawDepthImage)] = 2000.0f;
    legend_unit_[GetIndexFromCameraFlag(MLDepthCameraFlags_RawDepthImage)] = ' ';

    // Initialize depth camera related structures.
    MLDepthCameraSettingsInit(&camera_settings_);
    camera_settings_.streams = MLDepthCameraStream_ShortRange;
    camera_settings_.stream_configs[MLDepthCameraFrameType_LongRange].flags = MLDepthCameraFlags_DepthImage | MLDepthCameraFlags_Confidence | MLDepthCameraFlags_AmbientRawDepthImage | MLDepthCameraFlags_RawDepthImage;
    camera_settings_.stream_configs[MLDepthCameraFrameType_ShortRange].flags = MLDepthCameraFlags_DepthImage | MLDepthCameraFlags_Confidence | MLDepthCameraFlags_AmbientRawDepthImage | MLDepthCameraFlags_RawDepthImage;
    MLDepthCameraDataInit(&depth_camera_data_);
  }

  void OnStart() override {
    // Load legend bar textures.
    color_map_tex_ = Registry::GetInstance()->GetResourcePool()->LoadTexture("depth_gradient.png", GL_RGBA);
    conf_map_tex_ = Registry::GetInstance()->GetResourcePool()->LoadTexture("confidence_gradient.png", GL_RGBA);

    // Initialize head tracker to move preview with head.
    MLHandle head_tracker;
    UNWRAP_MLRESULT(MLHeadTrackingCreate(&head_tracker));
    UNWRAP_MLRESULT(MLHeadTrackingGetStaticData(head_tracker, &head_static_data_));
    SetHeadHandle(head_tracker); ///< This will cause Application class to destroy HT handle on app exit.


    const auto head_pose_opt = GetHeadPoseOrigin();
    if (!head_pose_opt.has_value()) {
      ALOGE("No head pose available at application start! For best experience, start the application with the device on.");
    }
    const Pose head_pose = head_pose_opt.value_or(GetRoot()->GetWorldPose()).HorizontalRotationOnly();
    const Pose gui_offset(glm::vec3(.25f, 0.f, -2.f));  //> Make gui not obscure the preview too much
    GetGui().Place(head_pose + gui_offset);
  }

  void OnResume() override {
    if (ArePermissionsGranted()) {
      SetupRestrictedResources();
      GetGui().Show();
    }
  }

  void OnPause() override {
    if (update_settings_future_.valid()) {
      UNWRAP_MLRESULT(FinishUpdateSettingsAsync());
    }
    if (MLHandleIsValid(cam_context_)) {
      UNWRAP_MLRESULT(MLDepthCameraDisconnect(cam_context_));
      cam_context_ = ML_INVALID_HANDLE;
    }
  }

  void OnUpdate(float) override {
    UpdatePreview();
    UpdateGui();

    if (update_settings_future_.valid()) {
      const auto future_status = update_settings_future_.wait_for(0ms);
      if (future_status == std::future_status::ready) {
        UNWRAP_MLRESULT(FinishUpdateSettingsAsync());
      }
    }

    if (!update_settings_future_.valid()) {
      AcquireNewFrames();
    }
  }

  void OnStop() override {
    color_map_tex_.reset();
    conf_map_tex_.reset();
  }

private:
  void SetupRestrictedResources() {
    if (MLHandleIsValid(cam_context_)) {
      return;
    }
    ASSERT_MLRESULT(MLDepthCameraConnect(&camera_settings_, &cam_context_));
    UpdateStreamCapabilities();
    SetupPreview();
  }

  void UpdateGui() {
    auto &gui = GetGui();
    gui.BeginUpdate();
    bool is_running = true;
    if (gui.BeginDialog("Depth Camera", &is_running, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
      ImGui::TextDisabled("You can hide/show this GUI using bumper button!");

      ImGui::NewLine();
      ImGui::Text("Basic frame information");
      {
        ImGui::Text("\tCamera position xyz: (%.2f, %.2f, %.2f)", last_dcam_pose_.position.x, last_dcam_pose_.position.y, last_dcam_pose_.position.z);
        ImGui::Text("\tCamera rotation xyzw: (%.2f, %.2f, %.2f, %.2f)", last_dcam_pose_.rotation.x, last_dcam_pose_.rotation.y, last_dcam_pose_.rotation.z, last_dcam_pose_.rotation.w);
        ImGui::Text("\tFrame number: %ld", last_dcam_frame_number_);
        ImGui::Text("\tFrame time since device boot: %s", last_dcam_timestamp_str_);
        ImGui::Text("\tFrame type: %s", GetMLDepthCameraFrameTypeString(last_dcam_frametype_));
      }

      DrawIntrinsicDetails("Intrinsics:", last_dcam_intrinsics_);

      ImGui::NewLine();
      ImGui::Separator();
      ImGui::Text("Settings");
      {
        const bool are_settings_updated = update_settings_future_.valid();
        if (are_settings_updated) ImGui::BeginDisabled();

        ImGui::Text("Data type:"); ImGui::SameLine();
        if (ImGui::BeginTable("camera_flags_table", 2, ImGuiTableFlags_SizingStretchSame)) {
          ImGui::TableNextColumn();
          DrawDataTypeCheckbox(MLDepthCameraFlags_DepthImage);
          ImGui::TableNextColumn();
          DrawDataTypeCheckbox(MLDepthCameraFlags_Confidence);
          ImGui::TableNextColumn();
          DrawDataTypeCheckbox(MLDepthCameraFlags_AmbientRawDepthImage);
          ImGui::TableNextColumn();
          DrawDataTypeCheckbox(MLDepthCameraFlags_RawDepthImage);
        }
        ImGui::EndTable();

        ImGui::NewLine();
        ImGui::Text("Camera mode:"); ImGui::SameLine();
        DrawModeRadioButton(MLDepthCameraStream_ShortRange); ImGui::SameLine();
        DrawModeRadioButton(MLDepthCameraStream_LongRange);

        ImGui::NewLine();
        DrawControlSettings(static_cast<MLDepthCameraStream>(camera_settings_.streams));

        if (ImGui::Button("Apply")) {
          StartUpdateSettingsAsync();
        }

        ImGui::TextDisabled("Note: Use the button above to apply settings.");
        if (are_settings_updated) ImGui::EndDisabled();
      }
      ImGui::Separator();
      ImGui::NewLine();
      ImGui::Text("Marker Overlay");
      {
        ImGui::Checkbox("Show Overlay", &show_marker_overlay_);

        ImGui::ColorEdit3("Marker Color", &marker_color_[0]);

        ImGui::SliderFloat("Point Size", &marker_point_size_, 5.0f, 30.0f);

        ImGui::Text("Detected: %zu markers", detected_markers_.size());

        if (ImGui::CollapsingHeader("Marker Details")) {
          for (size_t m = 0; m < detected_markers_.size(); m++) {
            const auto& marker = detected_markers_[m];
            ImGui::Text("  [%zu] px(%.1f,%.1f) 3D(%.3f,%.3f,%.3f)m area=%d",
                        m,
                        marker.centroid_pixel.x, marker.centroid_pixel.y,
                        marker.position_camera[0],
                        marker.position_camera[1],
                        marker.position_camera[2],
                        marker.area_pixels);
          }
        }
      }
      ImGui::Separator();
      ImGui::NewLine();

      for (int i = 0; i < CAMERA_FLAGS_NO; i++) {
        CreateGuiGroup(i);
      }
    }
    gui.EndDialog();
    gui.EndUpdate();

    if (!is_running) {
      FinishActivity();
    }
  }

  void CreateGuiGroup(uint8_t idx) {
    const MLDepthCameraFlags camera_flag = GetCameraFlagFromIndex(idx);
    if (ImGui::CollapsingHeader(GetMLDepthCameraFlagsString(camera_flag))) {
      ImGui::PushID(idx);
      ImGui::Text("\tMinimum value in frame: %.1f", min_values_[idx]);
      ImGui::Text("\tMaximum value in frame: %.1f", max_values_[idx]);
      const bool is_slider_needed = camera_flag != MLDepthCameraFlags_Confidence;
      if (is_slider_needed) {
        ImGui::NewLine();
        ImGui::Text("Shader valid values range:");
        if (ImGui::SliderFloat("Minimum", &min_distance_[idx], 0.0, max_distance_[idx])) {
          UpdateMinDepth(idx);
          SetLegendText(idx);
        }
        if (ImGui::SliderFloat("Maximum", &max_distance_[idx], min_distance_[idx], distance_limit_[idx])) {
          UpdateMaxDepth(idx);
          SetLegendText(idx);
        }
      }
      ImGui::PopID();
    }
  }

  void UpdateMinDepth(const uint8_t idx) {
    auto depth_mat = std::static_pointer_cast<DepthMaterial>(shader_materials_[idx]);
    auto conf_mat = std::static_pointer_cast<ConfidenceMaterial>(shader_materials_[idx]);
    if (depth_mat) {
      depth_mat->SetMinDepth(min_distance_[idx]);
    } else if (conf_mat) {
      conf_mat->SetMinDepth(min_distance_[idx]);
    }
  }

  void UpdateMaxDepth(const uint8_t idx) {
    auto depth_mat = std::static_pointer_cast<DepthMaterial>(shader_materials_[idx]);
    auto conf_mat = std::static_pointer_cast<ConfidenceMaterial>(shader_materials_[idx]);
    if (depth_mat) {
      depth_mat->SetMaxDepth(max_distance_[idx]);
    } else if (conf_mat) {
      conf_mat->SetMaxDepth(max_distance_[idx]);
    }
  }

  void DrawControlSettings(MLDepthCameraStream stream) {
    const int frame_type = GetFrameTypeFromStream(stream);

    const auto& frame_rates = frame_rates_[frame_type];
    ImGui::Text("Frame rate:"); ImGui::SameLine();
    if (ImGui::BeginCombo("##framerate", frame_rates[cap_idx_])) {
      for (size_t i = 0; i < frame_rates.size(); i++) {
        const bool is_selected = static_cast<size_t>(cap_idx_) == i;
        if (ImGui::Selectable(frame_rates[i], is_selected)) {
          camera_settings_.stream_configs[frame_type].frame_rate = stream_caps_[frame_type][i].frame_rate;
          cap_idx_ = i;

          // After switching frame_rate, check exposure against current capabilities
          const auto& stream_cap = stream_caps_[frame_type][cap_idx_];
          auto& exposure = camera_settings_.stream_configs[frame_type].exposure;
          exposure = std::clamp(exposure, stream_cap.min_exposure, stream_cap.max_exposure);
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    const auto& stream_cap = stream_caps_[frame_type][cap_idx_];
    int exposure = camera_settings_.stream_configs[frame_type].exposure;
    ImGui::Text("Exposure:"); ImGui::SameLine();
    if (ImGui::SliderInt("##exposure", &exposure, stream_cap.min_exposure, stream_cap.max_exposure, "%dus")) {
      camera_settings_.stream_configs[frame_type].exposure = exposure;
    }
  }

  void DrawModeRadioButton(MLDepthCameraStream stream) {
    if (ImGui::RadioButton(GetMLDepthCameraStreamString(stream), camera_settings_.streams == stream) && camera_settings_.streams != stream) {
      camera_settings_.streams = stream;

      // Reset idx to the currently selected frame rate for current stream
      const auto frame_type = GetFrameTypeFromStream(stream);
      cap_idx_ = FindFrameRateIdx(frame_type, camera_settings_.stream_configs[frame_type].frame_rate);
    }
  }

  void DrawDataTypeCheckbox(MLDepthCameraFlags flag) {
    const auto config_idx = GetFrameTypeFromStream(static_cast<MLDepthCameraStream>(camera_settings_.streams));
    auto& flags_ref = camera_settings_.stream_configs[config_idx].flags;
    if (ImGui::CheckboxFlags(GetMLDepthCameraFlagsString(flag), &flags_ref, flag)) {
      StartUpdateSettingsAsync();
      const auto flag_idx = GetIndexFromCameraFlag(flag);
      const bool is_enabled = flags_ref & flag;

      // If we're disabling depth image, clear the depth texture, so our confidence shader doesn't
      // reuse old depth values.
      if (!is_enabled && flag == MLDepthCameraFlags_DepthImage) {
        glBindTexture(GL_TEXTURE_2D, texture_id_[flag_idx]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, texture_width_[flag_idx], texture_height_[flag_idx], 0, GL_RED, GL_FLOAT, blank_depth_tex_.data());
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      SetPreviewVisibility(flag_idx, is_enabled);
    }
  }

  void DrawIntrinsicDetails(const char* label, const MLDepthCameraIntrinsics& params) {
    if (ImGui::CollapsingHeader(label)) {
      ImGui::Text("Camera width: %d", params.width);
      ImGui::Text("Camera height: %d", params.height);
      ImGui::Text("Camera focal length: %.4f %.4f", params.focal_length.x,  params.focal_length.y);
      ImGui::Text("Camera principal point: %.4f %.4f", params.principal_point.x,  params.principal_point.y);
      ImGui::Text("Camera field of view: %.4f", params.fov);
      ImGui::Text("Camera distortion params k1, k2, p1, p2, k3:\n\t\t%.4f %.4f %.4f %.4f %.4f", params.distortion[0], params.distortion[1], params.distortion[2], params.distortion[3], params.distortion[4]);
    }
  }

  void UpdatePreview() {
    MLSnapshot *snapshot = nullptr;
    UNWRAP_MLRESULT(MLPerceptionGetSnapshot(&snapshot));
    MLTransform head_transform = {};
    UNWRAP_MLRESULT(MLSnapshotGetTransform(snapshot, &head_static_data_.coord_frame_head, &head_transform));
    UNWRAP_MLRESULT(MLPerceptionReleaseSnapshot(snapshot));
    grouped_node_->SetWorldPose(Pose{head_transform});
  }

  void DestroyPreview() {
    GetRoot()->RemoveChild(grouped_node_);
    grouped_node_.reset();
    for (auto& node : preview_nodes_) {
      node.reset();
    }
    for (auto& overlay : marker_overlay_nodes_) {
      overlay.reset();
    }
    for (auto& overlay : rejected_overlay_nodes_) {
      overlay.reset();
    }
    glDeleteTextures(texture_id_.size(), texture_id_.data());
    texture_id_.fill(0);
  }

  template <typename TMarker>
  std::shared_ptr<Mesh> CreateMarkerPointMeshInternal(
      const std::vector<TMarker>& markers,
      uint8_t quad_index) {

    std::vector<glm::vec3> vertices;
    // Reserve enough space for all blob pixels across all markers.
    size_t total_points = 0;
    for (const auto& marker : markers) {
      total_points += marker.pixels.size();
    }
    vertices.reserve(total_points);

    // Get texture dimensions for this quad
    float tex_width = static_cast<float>(texture_width_[quad_index]);
    float tex_height = static_cast<float>(texture_height_[quad_index]);

    for (const auto& marker : markers) {
      for (const auto& p : marker.pixels) {
        // Convert pixel coords to normalized quad space [-0.5, 0.5]
        float u_norm = (p.x / tex_width) - 0.5f;
        float v_norm = (p.y / tex_height) - 0.5f;

        glm::vec3 local_pos(u_norm, v_norm, 0.001f);  // Small z-offset
        vertices.push_back(local_pos);
      }
    }

    auto mesh = std::make_shared<Mesh>(Buffer::Category::Dynamic, GL_UNSIGNED_INT);
    mesh->UpdateMesh(vertices.data(), nullptr, vertices.size(), nullptr, 0);
    mesh->SetPrimitiveType(GL_POINTS);
    mesh->SetPointSize(marker_point_size_);
    return mesh;
  }

  std::shared_ptr<Mesh> CreateMarkerPointMesh(
      const std::vector<ml::marker_detection::DetectedMarker>& markers,
      uint8_t quad_index) {
    return CreateMarkerPointMeshInternal(markers, quad_index);
  }

  std::shared_ptr<Mesh> CreateMarkerPointMesh(
      const std::vector<ml::marker_detection::RejectedBlob>& markers,
      uint8_t quad_index) {
    return CreateMarkerPointMeshInternal(markers, quad_index);
  }

  void UpdateMarkerOverlay(uint8_t quad_index,
                           const std::vector<ml::marker_detection::DetectedMarker>& markers) {
    // Get parent node (first child of preview quad)
    auto children = preview_nodes_[quad_index]->GetChildren();
    if (children.empty()) {
      return;
    }
    auto parent_node = children[0];

    // Hide overlay if no markers or disabled
    if (markers.empty() || !show_marker_overlay_) {
      if (marker_overlay_nodes_[quad_index]) {
        parent_node->RemoveChild(marker_overlay_nodes_[quad_index]);
      }
      return;
    }

    // Create marker mesh
    auto marker_mesh = CreateMarkerPointMesh(markers, quad_index);

    // Create material
    auto marker_material = std::make_shared<MarkerOverlayMaterial>(
        marker_color_, 0.9f);

    // Create or update node
    if (!marker_overlay_nodes_[quad_index]) {
      auto renderable = std::make_shared<RenderableComponent>(
          marker_mesh, marker_material);
      marker_overlay_nodes_[quad_index] = std::make_shared<Node>();
      marker_overlay_nodes_[quad_index]->AddComponent(renderable);
      parent_node->AddChild(marker_overlay_nodes_[quad_index]);
    } else {
      auto renderable = marker_overlay_nodes_[quad_index]
          ->GetComponent<RenderableComponent>();
      renderable->SetMesh(marker_mesh);
      renderable->SetMaterial(marker_material);
      // Make sure it's added as child if not already
      parent_node->AddChild(marker_overlay_nodes_[quad_index]);
    }
  }

  void UpdateRejectedOverlay(uint8_t quad_index,
                             const std::vector<ml::marker_detection::RejectedBlob>& blobs) {
    // Get parent node (first child of preview quad)
    auto children = preview_nodes_[quad_index]->GetChildren();
    if (children.empty()) {
      return;
    }
    auto parent_node = children[0];

    // Hide overlay if no blobs or overlay is disabled
    if (blobs.empty() || !show_marker_overlay_) {
      if (rejected_overlay_nodes_[quad_index]) {
        parent_node->RemoveChild(rejected_overlay_nodes_[quad_index]);
      }
      return;
    }

    // Create marker mesh for rejected blobs
    auto marker_mesh = CreateMarkerPointMesh(blobs, quad_index);

    // Use a fixed green color for rejected blobs
    glm::vec3 rejected_color(0.0f, 1.0f, 0.0f);
    auto marker_material = std::make_shared<MarkerOverlayMaterial>(
        rejected_color, 0.9f);

    // Create or update node
    if (!rejected_overlay_nodes_[quad_index]) {
      auto renderable = std::make_shared<RenderableComponent>(
          marker_mesh, marker_material);
      rejected_overlay_nodes_[quad_index] = std::make_shared<Node>();
      rejected_overlay_nodes_[quad_index]->AddComponent(renderable);
      parent_node->AddChild(rejected_overlay_nodes_[quad_index]);
    } else {
      auto renderable = rejected_overlay_nodes_[quad_index]
          ->GetComponent<RenderableComponent>();
      renderable->SetMesh(marker_mesh);
      renderable->SetMaterial(marker_material);
      // Make sure it's added as child if not already
      parent_node->AddChild(rejected_overlay_nodes_[quad_index]);
    }
  }

  void ResizePreview(int w, int h, uint8_t idx) {
    if (w != texture_width_[idx] || h != texture_height_[idx]) {
      texture_width_[idx] = w;
      texture_height_[idx] = h;
      is_preview_invalid_ = true;
      if (idx == GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage)) {
        blank_depth_tex_.resize(w * h);
      }
    }
  }

  void SetupPreview() {
    if (texture_id_[0]) {
      DestroyPreview();
    }
    grouped_node_ = std::make_shared<Node>();

    const float scale = GetScale(CAMERA_FLAGS_NO) * 0.98f;
    const auto translations = GetTranslationGrid(CAMERA_FLAGS_NO);

    // Generate textures to write data to.
    glGenTextures(texture_id_.size(), texture_id_.data());
    CameraFlagsArray<std::shared_ptr<Texture>> texs;
    for (uint8_t i = 0; i < texs.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, texture_id_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, texture_width_[i], texture_height_[i], 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        texs[i] = std::make_shared<Texture>(GL_TEXTURE_2D, texture_id_[i], texture_width_[i], texture_height_[i]);
    }

    // Create specific materials.
    const auto depth_idx = GetIndexFromCameraFlag(MLDepthCameraFlags_DepthImage);
    const auto conf_idx = GetIndexFromCameraFlag(MLDepthCameraFlags_Confidence);
    const auto ambient_idx = GetIndexFromCameraFlag(MLDepthCameraFlags_AmbientRawDepthImage);
    const auto raw_idx = GetIndexFromCameraFlag(MLDepthCameraFlags_RawDepthImage);
    shader_materials_[depth_idx] = std::make_shared<DepthMaterial>(texs[depth_idx], color_map_tex_, min_distance_[depth_idx], max_distance_[depth_idx]);
    shader_materials_[conf_idx] = std::make_shared<ConfidenceMaterial>(texs[depth_idx], texs[conf_idx], min_distance_[depth_idx], max_distance_[depth_idx]);
    shader_materials_[ambient_idx] = std::make_shared<DepthMaterial>(texs[ambient_idx], color_map_tex_, min_distance_[ambient_idx], max_distance_[ambient_idx]);
    shader_materials_[raw_idx] = std::make_shared<DepthMaterial>(texs[raw_idx], color_map_tex_, min_distance_[raw_idx], max_distance_[raw_idx]);

    // Create preview nodes.
    CreatePreviewNode(color_map_tex_, depth_idx, scale, translations);
    CreatePreviewNode(conf_map_tex_, conf_idx, scale, translations);
    CreatePreviewNode(color_map_tex_, ambient_idx, scale, translations);
    CreatePreviewNode(color_map_tex_, raw_idx, scale, translations);

    // Group nodes together and add them to the root to make them render.
    for (auto& node : preview_nodes_) {
      grouped_node_->AddChild(node);
    }
    GetRoot()->AddChild(grouped_node_);
    is_preview_invalid_ = false;
  }

  void CreatePreviewNode(std::shared_ptr<Texture> legend_texture, uint8_t idx, const float scale, const std::vector<glm::vec3>& translations) {
    // Create preview quad.
    auto quad = Registry::GetInstance()->GetResourcePool()->GetMesh<QuadMesh>();
    auto material = shader_materials_[idx];
    material->SetPolygonMode(GL_FILL);
    auto gui_renderable = std::make_shared<RenderableComponent>(quad, material);
    auto gui_node = std::make_shared<Node>();
    gui_node->SetLocalScale({1.f, -1.f * texture_height_[idx] / texture_width_[idx], 1.f}); // Negative vertical scale to flip the image.
    gui_node->AddComponent(gui_renderable);
    preview_nodes_[idx] = std::make_shared<Node>();
    preview_nodes_[idx]->AddChild(gui_node);

    // Create a colored bar legend over the preview.
    auto combo_node = std::make_shared<Node>();
    auto legend_quad = Registry::GetInstance()->GetResourcePool()->GetMesh<QuadMesh>();
    auto legend_mat = std::make_shared<TexturedMaterial>(legend_texture);
    legend_mat->SetPolygonMode(GL_FILL);
    auto legend_renderable = std::make_shared<RenderableComponent>(legend_quad, legend_mat);
    auto legend_node = std::make_shared<Node>();
    legend_node->SetLocalScale({1.f, 1.f / 51.2f, 1.f});
    legend_node->AddComponent(legend_renderable);

    // Create a text node under the legend bar.
    legend_text_nodes_[idx] = ml::app_framework::CreatePresetNode(ml::app_framework::NodeType::Text);
    SetLegendText(idx);
    legend_text_nodes_[idx]->SetLocalTranslation(glm::vec3{-0.505f, -0.02f, 0.f});
    constexpr auto text_scale = 0.02f / 8.f;
    legend_text_nodes_[idx]->SetLocalScale(glm::vec3{text_scale, -text_scale, 1.f});

    // Group legend-related nodes together.
    combo_node->AddChild(legend_node);
    combo_node->AddChild(legend_text_nodes_[idx]);
    combo_node->SetLocalTranslation(glm::vec3{0.f, 0.5f, 0.f});

    // Set top preview node.
    preview_nodes_[idx]->AddChild(combo_node);
    preview_nodes_[idx]->SetLocalPose({translations[idx]});
    preview_nodes_[idx]->SetLocalScale({scale, scale, 1.f});

    const auto frame_type = GetFrameTypeFromStream(static_cast<MLDepthCameraStream>(camera_settings_.streams));
    const auto & flags = camera_settings_.stream_configs[frame_type].flags;
    SetPreviewVisibility(idx, flags & GetCameraFlagFromIndex(idx));
  }

  void SetLegendText(uint8_t idx) {
    char legend_label[75];
    const float range_min = min_distance_[idx];
    const float range_max = max_distance_[idx];
    const char unit = legend_unit_[idx];
    snprintf(legend_label, 75, "%1.1f%c%32.1f%c%30.1f%c", range_min, unit, (range_max - range_min) / 2.f, unit, range_max, unit);
    legend_text_nodes_[idx]->GetComponent<ml::app_framework::TextComponent>()->SetText(legend_label);
  }

  void SetPreviewVisibility(uint8_t idx, bool is_visible) {
    for (auto& child : preview_nodes_[idx]->GetChildren()) {
      SetComponentVisibility(child, is_visible); ///< Set visibility for nodes that are preview quads.
      for (auto& subchild : child->GetChildren()) {
        SetComponentVisibility(subchild, is_visible); ///< Set visibility for both legend-related subnodes.
      }
    }
  }

  void SetComponentVisibility(std::shared_ptr<Node> node, bool is_visible) {
    auto comp = node->GetComponent<RenderableComponent>();
    if (comp) {
      comp->SetVisible(is_visible);
    }
  }

  void AcquireNewFrames() {
    const auto res = MLDepthCameraGetLatestDepthData(cam_context_, 0, &depth_camera_data_); ///< Passing 0 timeout to make the call nonblocking.
    if (res == MLResult_Ok) {
      // Check and update basic frame data.
      UpdateFrameData(depth_camera_data_);
      CheckFrameNumber(depth_camera_data_);

      // Submit data to the shader.
      UpdateImage(depth_camera_data_, MLDepthCameraFlags_DepthImage);
      UpdateImage(depth_camera_data_, MLDepthCameraFlags_Confidence);
      UpdateImage(depth_camera_data_, MLDepthCameraFlags_AmbientRawDepthImage);
      UpdateImage(depth_camera_data_, MLDepthCameraFlags_RawDepthImage);

      processFrame();

      UNWRAP_MLRESULT(MLDepthCameraReleaseDepthData(cam_context_, &depth_camera_data_)); ///< Always remember to release the data, otherwise buffer overload will happen.
    } else if (res != MLResult_Timeout) { ///< Need to check for timeout, especially when nonblocking.
        UNWRAP_MLRESULT(res);
    }
  }

  void UpdateImage(MLDepthCameraData& data, MLDepthCameraFlags data_flag) {
    const auto idx = GetIndexFromCameraFlag(data_flag);
    auto data_map = GetCameraFrameBuffer(data, data_flag);
    // Check if the frame buffer is valid (could also check camera_settings_.flags)
    if (data_map == nullptr || data_map->size == 0) {
      return;
    }

    // All depth data in this app is float type, just making sure that this is right and our vectors won't break.
    constexpr auto type_size = sizeof(float);
    if (data_map->bytes_per_unit != type_size) {
      ALOGE("Bytes per pixel equal to %d, instead of %ld! Data alignment mismatch!", data_map->bytes_per_unit, type_size);
      FinishActivity();
      return;
    }

    ResizePreview(data_map->stride / type_size, data_map->height, idx);
    SetNewFrame(idx, data_map->data);

    // Get min/max values
    float* data_float_ptr = reinterpret_cast<float*>(data_map->data);
    const size_t data_size = data_map->stride / type_size * data_map->height;
    const auto [min, max] = std::minmax_element(data_float_ptr, data_float_ptr + data_size);
    min_values_[idx] = *min;
    max_values_[idx] = *max;
  }

  void UpdateFrameData(MLDepthCameraData& data) {
    if (data.frame_count == 0) {
      return;
    }
    last_dcam_pose_ = data.frames[0].camera_pose;
    last_dcam_frametype_ = data.frames[0].frame_type;
    last_dcam_intrinsics_ = data.frames[0].intrinsics;
    GetMLTimeString(data.frames[0].frame_timestamp, last_dcam_timestamp_str_, last_dcam_timestamp_str_size_);
  }

  void GetMLTimeString(const MLTime time, char* cstr, size_t size) {
    timespec ts = {};
    UNWRAP_MLRESULT(MLTimeConvertMLTimeToSystemTime(time, &ts));
    const auto hours_div = std::lldiv(ts.tv_sec, 60 * 60);
    const auto mins_div = std::lldiv(hours_div.rem, 60);
    snprintf(cstr, size, "%lld:%02lld:%02lld", hours_div.quot, mins_div.quot, mins_div.rem);

  }

  void CheckFrameNumber(MLDepthCameraData& data) {
    if (data.frame_count == 0) {
      return;
    }
    const int64_t current_frame_number = data.frames[0].frame_number;
    if (last_dcam_frame_number_ >= 0) {
      const int64_t diff = current_frame_number - last_dcam_frame_number_;
      ALOGD("Frame diff: %ld", diff);
      if (diff > 1) {
        ALOGV("Lost frames! Current frame: %ld, last frame: %ld, difference: %ld.", current_frame_number, last_dcam_frame_number_, diff);
      }
    }
    last_dcam_frame_number_ = current_frame_number;
  }

  void SetNewFrame(uint8_t idx, void* data) {
    if (is_preview_invalid_) {
      SetupPreview();
    }
    glBindTexture(GL_TEXTURE_2D, texture_id_[idx]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, texture_width_[idx], texture_height_[idx], 0, GL_RED, GL_FLOAT, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  uint8_t GetIndexFromCameraFlag(MLDepthCameraFlags flag) {
    switch(flag) {
      case MLDepthCameraFlags_DepthImage: return 0;
      case MLDepthCameraFlags_Confidence: return 1;
      case MLDepthCameraFlags_AmbientRawDepthImage: return 2;
      case MLDepthCameraFlags_RawDepthImage: return 3;
      default:
        ALOGE("Unsupported flag used! Returning 0 index to not crash.");
        return 0;
    }
  }

  MLDepthCameraFlags GetCameraFlagFromIndex(uint8_t idx) {
    switch(idx) {
      case 0: return MLDepthCameraFlags_DepthImage;
      case 1: return MLDepthCameraFlags_Confidence;
      case 2: return MLDepthCameraFlags_AmbientRawDepthImage;
      case 3: return MLDepthCameraFlags_RawDepthImage;
      default:
        ALOGE("Unsupported index used! Returning DepthImage flag to not crash.");
        return MLDepthCameraFlags_DepthImage;
    }
  }

  MLDepthCameraFrameBuffer* GetCameraFrameBuffer(MLDepthCameraData& data, MLDepthCameraFlags flag) {
    if (data.frame_count == 0) {
      return nullptr;
    }
    switch(flag) {
      case MLDepthCameraFlags_DepthImage: return data.frames[0].depth_image;
      case MLDepthCameraFlags_Confidence: return data.frames[0].confidence;
      case MLDepthCameraFlags_AmbientRawDepthImage: return data.frames[0].ambient_raw_depth_image;
      case MLDepthCameraFlags_RawDepthImage: return data.frames[0].raw_depth_image;
      default:
        ALOGE("Unsupported flag used, can't return valid frame buffer! Returning nullptr.");
        return nullptr;
    }
  }

  MLDepthCameraFrameType GetFrameTypeFromStream(MLDepthCameraStream stream) {
    switch(stream) {
      case MLDepthCameraStream_LongRange: return MLDepthCameraFrameType_LongRange;
      case MLDepthCameraStream_ShortRange: return MLDepthCameraFrameType_ShortRange;
      default:
        ALOGE("Unsupported stream used! Returning LongRange to not crash.");
        return MLDepthCameraFrameType_LongRange;
    }
  }

  MLDepthCameraStream GetStreamFromFrameType(MLDepthCameraFrameType frame_type) {
    switch(frame_type) {
      case MLDepthCameraFrameType_LongRange: return MLDepthCameraStream_LongRange;
      case MLDepthCameraFrameType_ShortRange: return MLDepthCameraStream_ShortRange;
      default:
        ALOGE("Unsupported frame_type used! Returning LongRange to not crash.");
        return MLDepthCameraStream_LongRange;
    }
  }

  void StartUpdateSettingsAsync() {
    update_settings_future_ = std::async(std::launch::async, MLDepthCameraUpdateSettings, cam_context_, &camera_settings_);
  }

  MLResult FinishUpdateSettingsAsync() {
    UNWRAP_RET_MLRESULT(update_settings_future_.get());
    return MLResult_Ok;
  }

  void processFrame() {
    ALOGI("processFrame called with frame_count: %u", depth_camera_data_.frame_count);

    if (depth_camera_data_.frame_count >= 1) {
      for (size_t i = 0; i < depth_camera_data_.frame_count; i++) {
        ALOGI("=== Frame %zu ===", i);

        // Process Raw Depth Image
        if (depth_camera_data_.frames[i].raw_depth_image != nullptr) {
          auto* buffer = depth_camera_data_.frames[i].raw_depth_image;
          ALOGI("Raw Depth Image: %ux%u pixels", buffer->width, buffer->height);

          float* data = reinterpret_cast<float*>(buffer->data);
          if (data != nullptr && buffer->width > 0 && buffer->height > 0) {
            // Print first 5 pixel values
            ALOGI("  First 5 pixels: %.2f, %.2f, %.2f, %.2f, %.2f",
                  data[0], data[1], data[2], data[3], data[4]);

            // Calculate min/max for center pixel region (100 pixels)
            int center_x = buffer->width / 2;
            int center_y = buffer->height / 2;
            int start_idx = center_y * buffer->width + center_x - 50;
            float min_val = data[start_idx];
            float max_val = data[start_idx];
            for (int j = 0; j < 100 && (start_idx + j) < (buffer->width * buffer->height); j++) {
              min_val = std::min(min_val, data[start_idx + j]);
              max_val = std::max(max_val, data[start_idx + j]);
            }
            ALOGI("  Center region min: %.2f, max: %.2f", min_val, max_val);
          }
        } else {
          ALOGI("Raw Depth Image: NULL");
        }

        // Process Processed Depth Image
        if (depth_camera_data_.frames[i].depth_image != nullptr) {
          auto* buffer = depth_camera_data_.frames[i].depth_image;
          ALOGI("Processed Depth Image: %ux%u pixels", buffer->width, buffer->height);

          float* data = reinterpret_cast<float*>(buffer->data);
          if (data != nullptr && buffer->width > 0 && buffer->height > 0) {
            // Print first 5 pixel values (in meters)
            ALOGI("  First 5 pixels (meters): %.3f, %.3f, %.3f, %.3f, %.3f",
                  data[0], data[1], data[2], data[3], data[4]);

            // Calculate min/max for center pixel region
            int center_x = buffer->width / 2;
            int center_y = buffer->height / 2;
            int start_idx = center_y * buffer->width + center_x - 50;
            float min_val = data[start_idx];
            float max_val = data[start_idx];
            for (int j = 0; j < 100 && (start_idx + j) < (buffer->width * buffer->height); j++) {
              if (data[start_idx + j] > 0.0f) {  // Only consider valid depth values
                min_val = std::min(min_val, data[start_idx + j]);
                max_val = std::max(max_val, data[start_idx + j]);
              }
            }
            ALOGI("  Center region depth (m): min=%.3f, max=%.3f", min_val, max_val);
          }
        } else {
          ALOGI("Processed Depth Image: NULL");
        }

        // Process Confidence Scores
        if (depth_camera_data_.frames[i].confidence != nullptr) {
          auto* buffer = depth_camera_data_.frames[i].confidence;
          ALOGI("Confidence Buffer: %ux%u pixels", buffer->width, buffer->height);

          float* data = reinterpret_cast<float*>(buffer->data);
          if (data != nullptr && buffer->width > 0 && buffer->height > 0) {
            // Print first 5 confidence values
            ALOGI("  First 5 confidence values: %.2f, %.2f, %.2f, %.2f, %.2f",
                  data[0], data[1], data[2], data[3], data[4]);

            // Calculate min/max/average for center region
            int center_x = buffer->width / 2;
            int center_y = buffer->height / 2;
            int start_idx = center_y * buffer->width + center_x - 50;
            float min_val = data[start_idx];
            float max_val = data[start_idx];
            float sum = 0.0f;
            int count = 0;
            for (int j = 0; j < 100 && (start_idx + j) < (buffer->width * buffer->height); j++) {
              min_val = std::min(min_val, data[start_idx + j]);
              max_val = std::max(max_val, data[start_idx + j]);
              sum += data[start_idx + j];
              count++;
            }
            float avg = sum / count;
            ALOGI("  Center region confidence: min=%.2f, max=%.2f, avg=%.2f", min_val, max_val, avg);
          }
        } else {
          ALOGI("Confidence Buffer: NULL");
        }

        // Process Ambient Raw Depth (for reference)
        if (depth_camera_data_.frames[i].ambient_raw_depth_image != nullptr) {
          auto* buffer = depth_camera_data_.frames[i].ambient_raw_depth_image;
          ALOGI("Ambient Raw Depth: %ux%u pixels", buffer->width, buffer->height);
        } else {
          ALOGI("Ambient Raw Depth: NULL");
        }

        // ========== MARKER DETECTION INTEGRATION ==========
        if (depth_camera_data_.frames[i].raw_depth_image != nullptr &&
            depth_camera_data_.frames[i].depth_image != nullptr) {

          auto* raw_buffer = depth_camera_data_.frames[i].raw_depth_image;
          auto* depth_buffer = depth_camera_data_.frames[i].depth_image;
          auto* ambient_buffer = depth_camera_data_.frames[i].ambient_raw_depth_image;

          float* raw_data = reinterpret_cast<float*>(raw_buffer->data);
          float* depth_data = reinterpret_cast<float*>(depth_buffer->data);
          float* ambient_data = ambient_buffer ?
                                reinterpret_cast<float*>(ambient_buffer->data) : nullptr;

          if (raw_data && depth_data && raw_buffer->width > 0 && raw_buffer->height > 0) {
            const auto& intrinsics = depth_camera_data_.frames[i].intrinsics;

            ml::marker_detection::MarkerDetectionConfig config;
            config.intensity_threshold_min = 1280.0f;
            config.intensity_threshold_max = 2000.0f;
            config.use_ambient_subtraction = false;  // Set to true to test ambient subtraction

            rejected_blobs_.clear();
            detected_markers_ = ml::marker_detection::MarkerDetection::detectMarkerPositions(
                raw_data, depth_data, ambient_data,
                raw_buffer->width, raw_buffer->height,
                intrinsics, config, &rejected_blobs_
            );

            ALOGI("Detected %zu IR markers", detected_markers_.size());
            for (size_t m = 0; m < detected_markers_.size(); m++) {
              const auto& marker = detected_markers_[m];
              ALOGI("  Marker %zu: pixel(%.1f, %.1f) 3D(%.3f, %.3f, %.3f)m area=%d intensity=%.1f",
                    m,
                    marker.centroid_pixel.x, marker.centroid_pixel.y,
                    marker.position_camera[0], marker.position_camera[1],
                    marker.position_camera[2],
                    marker.area_pixels, marker.intensity);
            }

            // Update overlay on raw depth image quad
            const auto raw_idx = GetIndexFromCameraFlag(
                MLDepthCameraFlags_RawDepthImage);
            UpdateMarkerOverlay(raw_idx, detected_markers_);
            UpdateRejectedOverlay(raw_idx, rejected_blobs_);
          }
        }
        // ========== END MARKER DETECTION ==========
      }
    }
  }

  void UpdateStreamCapabilities() {
    MLDepthCameraCapabilityFilter filter;
    MLDepthCameraCapabilityFilterInit(&filter);
    MLDepthCameraCapabilityList cap_list;
    for (int frame_type = 0; frame_type < MLDepthCameraFrameType_Count; frame_type++) {
      const auto stream = GetStreamFromFrameType(static_cast<MLDepthCameraFrameType>(frame_type));
      filter.streams = stream;
      UNWRAP_MLRESULT(MLDepthCameraGetCapabilities(cam_context_, &filter, &cap_list));

      //  clear frame_rates_ and stream_caps_
      frame_rates_[frame_type].clear();
      stream_caps_[frame_type].clear();

      for (size_t i = 0; i < cap_list.size; i++) {
        const auto configuration = cap_list.capabilities[i];
        if (configuration.size <= 0) {
          continue;
        }
        // We're quering only single Stream at a time via filter.streams, so it will have at max 1 elem
        const auto stream_cap = configuration.stream_capabilities[0];
        stream_caps_[frame_type].push_back(stream_cap);
        const char* frame_rate_str = GetMLDepthCameraFrameRateString(stream_cap.frame_rate);
        frame_rates_[frame_type].push_back(frame_rate_str);

        // Check if current element matches currently set frame rate
        if (stream & camera_settings_.streams) {
          const auto current_fr = camera_settings_.stream_configs[frame_type].frame_rate;
          if (current_fr == stream_cap.frame_rate) {
            cap_idx_ = i;
          }
        }
      }
      UNWRAP_MLRESULT(MLDepthCameraReleaseCapabilities(cam_context_, &cap_list));
    }
  }

  int FindFrameRateIdx(MLDepthCameraFrameType frame_type, MLDepthCameraFrameRate frame_rate) {
    const auto& stream_caps = stream_caps_[frame_type];
    for (size_t i = 0; i < stream_caps.size(); i++) {
      if (stream_caps[i].frame_rate == frame_rate) {
        return i;
      }
    }
    return 0;
  }

  

  static const uint8_t CAMERA_FLAGS_NO = 4;
  template<typename T>
  using CameraFlagsArray = typename std::array<T, CAMERA_FLAGS_NO>;

  CameraFlagsArray<GLuint> texture_id_;
  CameraFlagsArray<int32_t> texture_width_, texture_height_;
  CameraFlagsArray<std::shared_ptr<Node>> preview_nodes_;
  CameraFlagsArray<std::shared_ptr<Material>> shader_materials_;
  CameraFlagsArray<std::shared_ptr<Node>> legend_text_nodes_;
  CameraFlagsArray<float> max_distance_, min_distance_, distance_limit_;
  CameraFlagsArray<char> legend_unit_;
  std::vector<float> blank_depth_tex_;

  MLHandle cam_context_;
  MLHeadTrackingStaticData head_static_data_;
  MLDepthCameraSettings camera_settings_;
  MLDepthCameraData depth_camera_data_;

  std::shared_ptr<Texture> color_map_tex_, conf_map_tex_;
  std::shared_ptr<Node> grouped_node_;

  MLDepthCameraFrameType last_dcam_frametype_;
  MLDepthCameraIntrinsics last_dcam_intrinsics_;
  int64_t last_dcam_frame_number_;
  MLTransform last_dcam_pose_;
  char last_dcam_timestamp_str_[100];
  const size_t last_dcam_timestamp_str_size_ = sizeof(last_dcam_timestamp_str_);

  CameraFlagsArray<float> min_values_;
  CameraFlagsArray<float> max_values_;
  bool is_preview_invalid_;
  std::future<MLResult> update_settings_future_;
  std::array<std::vector<MLDepthCameraStreamCapability>, MLDepthCameraFrameType_Count> stream_caps_;
  std::array<std::vector<const char*>, MLDepthCameraFrameType_Count> frame_rates_;
  int8_t cap_idx_;

  // Marker overlay visualization
  std::vector<ml::marker_detection::DetectedMarker> detected_markers_;
  CameraFlagsArray<std::shared_ptr<Node>> marker_overlay_nodes_;
  std::vector<ml::marker_detection::RejectedBlob> rejected_blobs_;
  CameraFlagsArray<std::shared_ptr<Node>> rejected_overlay_nodes_;
  bool show_marker_overlay_ = true;
  glm::vec3 marker_color_ = glm::vec3(1.0f, 0.0f, 0.0f);  // Red
  float marker_point_size_ = 15.0f;
};

void android_main(struct android_app *state) {
  DepthCameraApp app(state);
  app.RunApp();
}
