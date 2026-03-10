#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "tool_definition.h"
#include <app_framework/toolset.h>
#include <algorithm>

namespace ml {
namespace tool_tracking {

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
    if (tool_index_.count(identifier) > 0) return false;

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

    tool_index_.insert({identifier, static_cast<int>(tools_.size())});
    tools_.push_back(tool);

    ALOGI("[ToolTracker] Added tool '%s' (%d spheres, r=%.1fmm)",
          identifier.c_str(), tool.num_spheres, radius_mm);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// RemoveTool
// ─────────────────────────────────────────────────────────────────────────────
bool ToolTracker::RemoveTool(const std::string& identifier) {
    if (tool_index_.count(identifier) == 0) return false;

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
    return true;
}

void ToolTracker::RemoveAllTools() {
    tools_.clear();
    tool_index_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// GetToolTransform
// ─────────────────────────────────────────────────────────────────────────────
cv::Mat ToolTracker::GetToolTransform(const std::string& identifier) const {
    if (tools_.empty() || tool_index_.count(identifier) == 0)
        return cv::Mat::zeros(8, 1, CV_32F);
    return tools_.at(tool_index_.at(identifier)).cur_transform.clone();
}

} // namespace tool_tracking
} // namespace ml
