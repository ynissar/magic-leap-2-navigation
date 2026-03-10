#pragma once

#include <string>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
#include "marker_detection.h"

namespace ml {
namespace tool_tracking {

// A directed edge between two detected sphere indices, with the measured distance.
struct Side {
    int id_from{0};
    int id_to{0};
    float distance{0.f};

    static bool compare(const Side& a, const Side& b) {
        return a.distance < b.distance;
    }
};

// One candidate pose solution found during the graph search for a single tool.
struct ToolResult {
    int tool_id{-1};
    std::vector<int> sphere_ids;      // indices into the current frame's detected_markers
    float error{0.f};                 // summed side-length error (mm)
    std::vector<int> occluded_nodes;  // tool node indices that were not visible

    // Sort: fewest occluded first, then lowest total error.
    static bool compare(const ToolResult& a, const ToolResult& b) {
        if (a.occluded_nodes.size() != b.occluded_nodes.size())
            return a.occluded_nodes.size() < b.occluded_nodes.size();
        return a.error < b.error;
    }
};

struct ToolResultContainer {
    int tool_id{-1};
    std::vector<ToolResult> candidates;
};

// A registered rigid-body tool, defined by its sphere template geometry.
struct TrackedTool {
    std::string identifier;
    int num_spheres{0};
    int min_visible_spheres{3};
    float sphere_radius_mm{5.f};

    cv::Mat3f spheres_xyz_mm;        // (num_spheres × 1) local-frame positions in mm
    cv::Mat   map;                   // (num_spheres × num_spheres) upper-triangular pairwise distance matrix (mm)
    std::vector<Side> ordered_sides; // sorted by distance for fast seed search

    // 8×1: [x, y, z (m),  qx, qy, qz, qw,  valid(0/1)] in camera space
    cv::Mat cur_transform = cv::Mat::zeros(8, 1, CV_32F);

    float lowpass_factor_position{0.6f}; // blend factor toward new position (0=frozen, 1=raw)
    float lowpass_factor_rotation{0.3f}; // blend factor toward new rotation (slerp t)

    bool tracking_finished{true};
};

// ─────────────────────────────────────────────────────────────────────────────
// ToolTracker — main public API
// ─────────────────────────────────────────────────────────────────────────────

class ToolTracker {
public:
    // Register a tool defined by its sphere positions in the tool-local frame (mm).
    // min_visible: minimum number of spheres that must be detected for a valid pose.
    bool AddTool(cv::Mat3f spheres_mm, float radius_mm,
                 const std::string& identifier,
                 int min_visible = 3);

    bool RemoveTool(const std::string& identifier);
    void RemoveAllTools();

    // Process one frame of detected markers.  Call this once per depth frame,
    // immediately after MarkerDetection::detectMarkerPositions().
    void ProcessFrame(const std::vector<ml::marker_detection::DetectedMarker>& markers);

    // Returns the 8×1 transform for the named tool:
    //   [x, y, z (meters),  qx, qy, qz, qw,  valid]
    // valid == 1.0 means the tool was successfully tracked this frame.
    cv::Mat GetToolTransform(const std::string& identifier) const;

    // Tune matching tolerances (mm).
    void SetTolerances(float side_mm, float avg_mm) {
        tolerance_side_ = side_mm;
        tolerance_avg_  = avg_mm;
    }

private:
    // Build the upper-triangular pairwise-distance map and a sorted side list.
    void ConstructMap(const cv::Mat3f& spheres_mm, int n,
                      cv::Mat& out_map, std::vector<Side>& out_sides) const;

    // Graph search: find all candidate sphere assignments for one tool.
    void TrackTool(TrackedTool& tool,
                   const cv::Mat3f& frame_spheres_mm,
                   const cv::Mat& frame_map,
                   const std::vector<Side>& frame_sides,
                   int num_frame_spheres,
                   ToolResultContainer& result) const;

    // Greedy conflict resolution across all tools.
    void UnionSegmentation(ToolResultContainer* raw, int num_tools,
                           const cv::Mat3f& frame_spheres_mm);

    // Kabsch algorithm: compute 6DOF pose from matched point sets.
    // Returns 8×1 Mat [xyz(m), qxyzw, valid], or zeros on failure.
    cv::Mat MatchPointsKabsch(TrackedTool& tool,
                              const cv::Mat3f& frame_spheres_mm,
                              const std::vector<int>& sphere_ids,
                              const std::vector<int>& occluded_nodes);

    std::vector<TrackedTool>    tools_;
    std::map<std::string, int>  tool_index_;

    float tolerance_side_{4.f}; // mm — max allowed per-side error
    float tolerance_avg_ {4.f}; // mm — max allowed mean error for a candidate
};

} // namespace tool_tracking
} // namespace ml
