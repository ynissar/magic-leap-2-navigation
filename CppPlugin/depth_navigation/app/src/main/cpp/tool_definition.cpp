#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "tool_definition.h"
#include <app_framework/toolset.h>
#include <algorithm>
#include <android/log.h>

namespace ml {
namespace tool_tracking {

// ─────────────────────────────────────────────────────────────────────────────
// LogToolDefinition
// Emits tool geometry to its own logcat tag "ToolDef" so it can be read
// independently of the main app log:
//   adb logcat -s ToolDef
// ─────────────────────────────────────────────────────────────────────────────
#define TOOL_DEF_LOG(...) \
    __android_log_print(ANDROID_LOG_INFO, "ToolDef", __VA_ARGS__)

static void LogToolDefinition(const TrackedTool& tool) {
    TOOL_DEF_LOG("=== tool: %s  spheres: %d  radius: %.1f mm ===",
                 tool.identifier.c_str(), tool.num_spheres, tool.sphere_radius_mm);

    // Sphere positions.
    for (int i = 0; i < tool.num_spheres; ++i) {
        const cv::Vec3f& p = tool.spheres_xyz_mm.at<cv::Vec3f>(i, 0);
        TOOL_DEF_LOG("  sphere[%d]  x=%.3f  y=%.3f  z=%.3f  (mm)",
                     i, p[0], p[1], p[2]);
    }

    // Sorted pairwise distances.
    TOOL_DEF_LOG("  ordered sides (id_from->id_to : dist mm):");
    for (const auto& s : tool.ordered_sides)
        TOOL_DEF_LOG("    %d -> %d : %.3f", s.id_from, s.id_to, s.distance);

    TOOL_DEF_LOG("=== end %s ===", tool.identifier.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// ConstructMap
// Builds the upper-triangular pairwise Euclidean distance matrix (mm) and a
// distance-sorted list of sides.  Ported from IRToolTracker::ConstructMap.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::ConstructMap(const cv::Mat3f& spheres_mm, int n,
                               cv::Mat& out_map,
                               std::vector<Side>& out_sides) const {
    for (int i = 0; i < n; ++i) {
        for (int j = i; j < n; ++j) {
            float dist = cv::norm(spheres_mm.at<cv::Vec3f>(i, 0) -
                                  spheres_mm.at<cv::Vec3f>(j, 0));
            out_map.at<float>(i, j) = dist;
            if (i == j) continue;

            Side s;
            s.id_from  = i;
            s.id_to    = j;
            s.distance = dist;

            // Insert into sorted list (ascending by distance)
            if (out_sides.empty() || dist >= out_sides.back().distance) {
                out_sides.push_back(s);
            } else if (dist <= out_sides.front().distance) {
                out_sides.insert(out_sides.begin(), s);
            } else {
                auto it = std::upper_bound(out_sides.cbegin(), out_sides.cend(),
                                           s, &Side::compare);
                out_sides.insert(it, s);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AddTool
// ─────────────────────────────────────────────────────────────────────────────
bool ToolTracker::AddTool(cv::Mat3f spheres_mm, float radius_mm,
                          const std::string& identifier, int min_visible) {
    if (tool_index_.count(identifier) > 0) {
        ALOGW("[ToolTracker] AddTool: tool '%s' already exists, ignoring",
              identifier.c_str());
        return false;
    }

    TrackedTool tool;
    tool.identifier          = identifier;
    tool.num_spheres         = spheres_mm.size().height;
    tool.sphere_radius_mm    = radius_mm;
    tool.spheres_xyz_mm      = spheres_mm.clone();
    tool.min_visible_spheres = std::max(3, std::min(min_visible, tool.num_spheres));

    cv::Mat map(cv::Size(tool.num_spheres, tool.num_spheres), CV_32F, cv::Scalar(0.f));
    std::vector<Side> sides;
    ConstructMap(spheres_mm, tool.num_spheres, map, sides);
    tool.map           = map.clone();
    tool.ordered_sides = sides;

    // Initialise one Kalman filter per sphere using the tool's noise parameters.
    for (int i = 0; i < tool.num_spheres; ++i)
        tool.sphere_kalman_filters.emplace_back(
            tool.kalman_measurement_noise,
            tool.kalman_position_noise,
            tool.kalman_velocity_noise);

    int new_index = static_cast<int>(tools_.size());
    tool_index_.insert({identifier, new_index});
    tools_.push_back(tool);

    ALOGI("[ToolTracker] Added tool '%s' (index=%d, spheres=%d, r=%.1fmm, min_visible=%d)",
          identifier.c_str(), new_index, tool.num_spheres, radius_mm,
          tool.min_visible_spheres);
    LogToolDefinition(tool);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// RemoveTool
// ─────────────────────────────────────────────────────────────────────────────
bool ToolTracker::RemoveTool(const std::string& identifier) {
    auto it = tool_index_.find(identifier);
    if (it == tool_index_.end()) {
        ALOGW("[ToolTracker] RemoveTool: unknown tool '%s'", identifier.c_str());
        return false;
    }

    int removed_index = it->second;
    ALOGI("[ToolTracker] Removing tool '%s' (index=%d)", identifier.c_str(), removed_index);

    // Rebuild tool list without the removed entry, keeping indices consistent.
    std::map<std::string, int> old_index(tool_index_);
    std::vector<TrackedTool>   old_tools(tools_);
    tools_.clear();
    tool_index_.erase(identifier);
    tool_index_.clear();
    for (auto& pair : old_index) {
        if (pair.first == identifier) continue;
        tool_index_.insert({pair.first, static_cast<int>(tools_.size())});
        tools_.push_back(old_tools.at(pair.second));
    }

    ALOGI("[ToolTracker] RemoveTool: now tracking %zu tools", tools_.size());
    return true;
}

void ToolTracker::RemoveAllTools() {
    ALOGI("[ToolTracker] RemoveAllTools: clearing %zu tools", tools_.size());
    tools_.clear();
    tool_index_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// GetToolTransform
// ─────────────────────────────────────────────────────────────────────────────
cv::Mat ToolTracker::GetToolTransform(const std::string& identifier) const {
    if (tools_.empty()) {
        ALOGV("[ToolTracker] GetToolTransform('%s'): no tools registered",
              identifier.c_str());
        return cv::Mat::zeros(8, 1, CV_32F);
    }

    auto it = tool_index_.find(identifier);
    if (it == tool_index_.end()) {
        ALOGW("[ToolTracker] GetToolTransform: unknown tool '%s'", identifier.c_str());
        return cv::Mat::zeros(8, 1, CV_32F);
    }

    const TrackedTool& t = tools_.at(it->second);
    ALOGV("[ToolTracker] GetToolTransform('%s'): ever_tracked=%d, valid_flag=%.1f",
          identifier.c_str(), t.ever_tracked ? 1 : 0, t.cur_transform.at<float>(7, 0));
    return t.cur_transform.clone();
}

} // namespace tool_tracking
} // namespace ml
