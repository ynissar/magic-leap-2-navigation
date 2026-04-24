#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "tool_definition.h"
#include <algorithm>
#include <android/log.h>
#include <app_framework/toolset.h>
#include <cmath>

namespace ml {
namespace tool_tracking {

// ─────────────────────────────────────────────────────────────────────────────
// TOOL_TRK_LOG
// Dedicated logcat tag for tool-tracking internals:
//   adb logcat -s ToolTrack
// ─────────────────────────────────────────────────────────────────────────────
#define TOOL_TRK_LOG(...)                                                      \
  __android_log_print(ANDROID_LOG_VERBOSE, "ToolTrack", __VA_ARGS__)

// ─────────────────────────────────────────────────────────────────────────────
// ProcessFrame
// Entry point called once per depth camera frame after marker detection.
// Converts detected marker positions to mm, builds a pairwise distance frame
// map, runs the graph search for each registered tool, then resolves conflicts.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::ProcessFrame(
    const std::vector<ml::marker_detection::DetectedMarker> &markers,
    const cv::Mat &camera_to_world_4x4) {
  camera_to_world_ = camera_to_world_4x4.clone();

  int num_frame_spheres = static_cast<int>(markers.size());
  if (num_frame_spheres < 3 || tools_.empty()) {
    TOOL_TRK_LOG("ProcessFrame: skipped (markers=%d, tools=%zu)",
                 num_frame_spheres, tools_.size());
    return;
  }

  TOOL_TRK_LOG("ProcessFrame: %d markers, %zu tools", num_frame_spheres,
               tools_.size());

  // Convert positions from metres to mm (all internal geometry is in mm)
  cv::Mat3f frame_spheres_mm(num_frame_spheres, 1);
  for (int i = 0; i < num_frame_spheres; ++i) {
    const cv::Vec3f &p = markers[i].position_camera;
    frame_spheres_mm.at<cv::Vec3f>(i, 0) =
        cv::Vec3f(p[0] * 1000.f, p[1] * 1000.f, p[2] * 1000.f);
  }

  // Build pairwise distance map for this frame
  cv::Mat frame_map(cv::Size(num_frame_spheres, num_frame_spheres), CV_32F,
                    cv::Scalar(0.f));
  std::vector<Side> frame_sides;
  ConstructMap(frame_spheres_mm, num_frame_spheres, frame_map, frame_sides);

  // Run graph search for every registered tool
  int num_tools = static_cast<int>(tools_.size());
  std::vector<ToolResultContainer> raw_results(num_tools);
  for (int i = 0; i < num_tools; ++i) {
    raw_results[i].tool_id = i;
    if (!tools_[i].tracking_finished) {
      TOOL_TRK_LOG("ProcessFrame: skipping tool %d ('%s') — previous tracking "
                   "not finished",
                   i, tools_[i].identifier.c_str());
      continue;
    }
    TOOL_TRK_LOG("ProcessFrame: tracking tool %d ('%s')", i,
                 tools_[i].identifier.c_str());
    TrackTool(tools_[i], frame_spheres_mm, frame_map, frame_sides,
              num_frame_spheres, raw_results[i]);
  }

  // Resolve conflicts and compute final poses
  UnionSegmentation(raw_results.data(), num_tools, frame_spheres_mm);
}

// ─────────────────────────────────────────────────────────────────────────────
// TrackTool
// Graph DFS search that matches detected frame spheres to the tool template.
// Ported directly from IRToolTracker::TrackTool, with mm units throughout.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::TrackTool(TrackedTool &tool,
                            const cv::Mat3f &frame_spheres_mm,
                            const cv::Mat &frame_map,
                            const std::vector<Side> &frame_sides,
                            int num_frame_spheres,
                            ToolResultContainer &result) const {
  tool.tracking_finished = false;

  if (num_frame_spheres < tool.min_visible_spheres) {
    TOOL_TRK_LOG(
        "TrackTool '%s': not enough markers this frame (have=%d, need>=%d)",
        tool.identifier.c_str(), num_frame_spheres, tool.min_visible_spheres);
    tool.tracking_finished = true;
    return;
  }

  int max_occluded = tool.num_spheres - tool.min_visible_spheres;

  // Pre-compute the mean template side length for the relative avg-tolerance
  // check. All pairwise distances in the template scale by the same depth-error
  // factor k, so the effective avg tolerance is max(tolerance_avg_,
  // tolerance_rel_ * mean_side).
  float mean_template_side_mm = 0.f;
  if (!tool.ordered_sides.empty()) {
    for (const Side &s : tool.ordered_sides)
      mean_template_side_mm += s.distance;
    mean_template_side_mm /= static_cast<float>(tool.ordered_sides.size());
  }
  const float eff_avg_tol =
      std::max(tolerance_avg_, tolerance_rel_ * mean_template_side_mm);

  struct search_entry {
    std::vector<int> visited_nodes_frame;
    float combined_error{0.f};
    int num_sides{0};
    std::vector<int> occluded_nodes_tool;
  };

  std::vector<search_entry> search_list;
  std::vector<Side> eligible_sides;
  std::vector<int> hidden_nodes;

  // Seed: find frame sides whose length matches the first visible template
  // side, accounting for possible leading occlusions.

  for (const Side &s : frame_sides) {
    TOOL_TRK_LOG("frame_side: %d -> %d : %.3f", s.id_from, s.id_to, s.distance);
  }
  for (const Side &s : tool.ordered_sides) {
    TOOL_TRK_LOG("tool.ordered_side: %d -> %d : %.3f", s.id_from, s.id_to,
                 s.distance);
  }
  TOOL_TRK_LOG("max_occluded: %d", max_occluded);

  for (int m = 0; m <= max_occluded; ++m) {
    std::vector<int> hidden_nodes_inside;
    for (int k = m + 1; k <= max_occluded + 1; ++k) {
      float cur_side_length = tool.map.at<float>(m, k);
      for (const Side &s : frame_sides) {
        if (std::abs(s.distance - cur_side_length) <
            std::max(tolerance_side_, tolerance_rel_ * cur_side_length))
          eligible_sides.push_back(s);
      }
      if (eligible_sides.empty() && max_occluded == 0) {
        TOOL_TRK_LOG(
            "TrackTool '%s': no eligible seed sides for m=%d (max_occluded=0)",
            tool.identifier.c_str(), m);
        tool.tracking_finished = true;
        return;
      }
      if (!eligible_sides.empty()) {
        for (int on = m + 1; on < k; ++on)
          hidden_nodes_inside.push_back(on);
        break;
      }
    }
    if (eligible_sides.empty() && max_occluded == 0) {
      TOOL_TRK_LOG(
          "TrackTool '%s': exhausted seeds with no matches (max_occluded=0)",
          tool.identifier.c_str());
      tool.tracking_finished = true;
      return;
    }
    if (!eligible_sides.empty()) {
      std::vector<int> all_hidden;
      all_hidden.insert(all_hidden.end(), hidden_nodes.begin(),
                        hidden_nodes.end());
      all_hidden.insert(all_hidden.end(), hidden_nodes_inside.begin(),
                        hidden_nodes_inside.end());
      if (static_cast<int>(all_hidden.size()) <= max_occluded) {
        for (const Side &s : eligible_sides) {
          search_list.push_back(
              {{s.id_from, s.id_to},
               std::abs(
                   s.distance -
                   tool.map.at<float>(
                       m,
                       m + 1 + static_cast<int>(hidden_nodes_inside.size()))),
               1,
               all_hidden});
          search_list.push_back(
              {{s.id_to, s.id_from},
               std::abs(
                   s.distance -
                   tool.map.at<float>(
                       m,
                       m + 1 + static_cast<int>(hidden_nodes_inside.size()))),
               1,
               all_hidden});
        }
      }
      eligible_sides.clear();
    }
    hidden_nodes.push_back(m);
  }

  // Expand search entries
  while (!search_list.empty()) {
    search_entry curr = search_list.back();
    search_list.pop_back();

    int visited = static_cast<int>(curr.visited_nodes_frame.size());
    int occluded = static_cast<int>(curr.occluded_nodes_tool.size());

    // Terminal condition: used all expected spheres
    if (occluded <= max_occluded && visited == (tool.num_spheres - occluded)) {
      if (curr.num_sides > 0 &&
          (curr.combined_error / curr.num_sides) < eff_avg_tol) {
        ToolResult r;
        r.error = curr.combined_error;
        r.sphere_ids = curr.visited_nodes_frame;
        r.occluded_nodes = curr.occluded_nodes_tool;
        result.candidates.push_back(r);
      }
      continue;
    }

    // Try to extend with each unvisited frame sphere
    for (int candidate = 0; candidate < num_frame_spheres; ++candidate) {
      if (std::find(curr.visited_nodes_frame.begin(),
                    curr.visited_nodes_frame.end(),
                    candidate) != curr.visited_nodes_frame.end())
        continue;

      bool exceeded = false;
      float err_new = 0.f;
      int err_cnt = 0;
      int tool_node_id = 0; // tool node id, skipping occluded nodes

      for (int j = 0; j < visited; ++j) {
        while (std::find(curr.occluded_nodes_tool.begin(),
                         curr.occluded_nodes_tool.end(),
                         tool_node_id) != curr.occluded_nodes_tool.end())
          ++tool_node_id;

        int marker_idx_a = curr.visited_nodes_frame[j];
        int marker_idx_b = candidate;
        if (marker_idx_a > marker_idx_b)
          std::swap(marker_idx_a, marker_idx_b);

        // next tool node index = visited count + occlusion count
        float expected = tool.map.at<float>(tool_node_id, visited + occluded);
        float err_side = std::abs(
            frame_map.at<float>(marker_idx_a, marker_idx_b) - expected);

        if (err_side > std::max(tolerance_side_, tolerance_rel_ * expected)) {
          exceeded = true;
          break;
        }
        err_new += err_side;
        ++err_cnt;
        ++tool_node_id;
      }

      if (exceeded) {
        // Optionally mark next tool node as occluded and retry without
        // candidate
        if (occluded < max_occluded) {
          std::vector<int> new_occl(curr.occluded_nodes_tool);
          new_occl.push_back(visited + occluded);
          search_list.push_back({curr.visited_nodes_frame, curr.combined_error,
                                 curr.num_sides, new_occl});
        }
        continue;
      }

      std::vector<int> new_visited(curr.visited_nodes_frame);
      new_visited.push_back(candidate);
      search_list.push_back({new_visited, curr.combined_error + err_new,
                             curr.num_sides + err_cnt,
                             curr.occluded_nodes_tool});
    }
  }

  TOOL_TRK_LOG("TrackTool '%s': %d candidates found", tool.identifier.c_str(),
               (int)result.candidates.size());
  tool.tracking_finished = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UnionSegmentation
// Greedy conflict resolution: sort all candidates by quality, take the best,
// then discard any other candidates that share a detected marker.
// Ported from IRToolTracker::UnionSegmentation.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::UnionSegmentation(ToolResultContainer *raw, int num_tools,
                                    const cv::Mat3f &frame_spheres_mm) {
  std::vector<ToolResult> pool;
  for (int i = 0; i < num_tools; ++i) {
    for (ToolResult &r : raw[i].candidates) {
      r.tool_id = i;
      pool.push_back(r);
    }
  }
  ALOGV("UnionSegmentation: total candidates across tools = %zu", pool.size());
  std::sort(pool.begin(), pool.end(), &ToolResult::compare);

  while (!pool.empty()) {
    ToolResult best = pool.front();
    pool.erase(pool.begin());

    cv::Mat result = MatchPointsKabsch(tools_[best.tool_id], frame_spheres_mm,
                                       best.sphere_ids, best.occluded_nodes);
    if (result.at<float>(7, 0) == 1.f) {
      tools_[best.tool_id].cur_transform = result.clone();
      tools_[best.tool_id].last_tracked_time = std::chrono::steady_clock::now();
      tools_[best.tool_id].ever_tracked = true;
      ALOGV("UnionSegmentation: tool %d ('%s') pose updated "
            "(visible_pts=%zu, occlusions=%zu, err=%.2f mm)",
            best.tool_id, tools_[best.tool_id].identifier.c_str(),
            best.sphere_ids.size(), best.occluded_nodes.size(), best.error);
    } else {
      ALOGV("UnionSegmentation: tool %d ('%s') Kabsch failed", best.tool_id,
            tools_[best.tool_id].identifier.c_str());
    }

    // Remove candidates that share any detected sphere with the chosen result
    size_t before = pool.size();
    pool.erase(std::remove_if(pool.begin(), pool.end(),
                              [&](const ToolResult &next) {
                                if (next.tool_id == best.tool_id)
                                  return true;
                                for (int cs : best.sphere_ids)
                                  for (int ns : next.sphere_ids)
                                    if (cs == ns)
                                      return true;
                                return false;
                              }),
               pool.end());
    ALOGV("UnionSegmentation: removed %zu conflicting candidates, %zu remain",
          before - pool.size(), pool.size());
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// MatchPointsKabsch
//
// Kabsch alignment for rigid registration between:
//   p: tool-template marker positions (known geometry, mm)
//   q: detected marker positions in the current frame (measured points, mm)
//
// Why this is used:
// Marker detection gives an unordered set of 3D points. To recover a full tool
// pose (rotation + translation), we need the rigid transform that best aligns
// the template to those measurements in a least-squares sense.
//
// The resulting camera-space transform is converted to world space using the
// current camera_to_world_ matrix. Position is emitted in metres for downstream
// ML2 consumers.
//
// Ported from IRToolTracker::MatchPointsKabsch, adapted for ML2 (no handedness
// flip).
// ─────────────────────────────────────────────────────────────────────────────
cv::Mat ToolTracker::MatchPointsKabsch(TrackedTool &tool,
                                       const cv::Mat3f &frame_spheres_mm,
                                       const std::vector<int> &sphere_ids,
                                       const std::vector<int> &occluded_nodes) {
  cv::Mat zeros = cv::Mat::zeros(8, 1, CV_32F);
  int num_pts = tool.num_spheres - static_cast<int>(occluded_nodes.size());
  if (num_pts < 3) {
    TOOL_TRK_LOG("MatchPointsKabsch '%s': not enough points (have=%d, need>=3)",
                 tool.identifier.c_str(), num_pts);
    return zeros;
  }

  cv::Mat p(num_pts, 3, CV_32F); // template points (mm)
  cv::Mat q(num_pts, 3, CV_32F); // detected points (mm)
  cv::Vec3f p_center(0.f), q_center(0.f);

  int tool_node_id = 0;
  for (int i = 0; i < num_pts; ++i) {
    while (std::find(occluded_nodes.begin(), occluded_nodes.end(),
                     tool_node_id) != occluded_nodes.end())
      ++tool_node_id;

    cv::Vec3f sp = tool.spheres_xyz_mm.at<cv::Vec3f>(tool_node_id, 0);
    p.at<float>(i, 0) = sp[0];
    p.at<float>(i, 1) = sp[1];
    p.at<float>(i, 2) = sp[2];
    p_center += sp;

    // Apply per-sphere Kalman filter before feeding into Kabsch.
    // tool_node_id tracks the tool-node index (skipping occluded nodes),
    // matching the sphere_kalman_filters vector layout exactly.
    cv::Vec3f squared_distance_raw =
        frame_spheres_mm.at<cv::Vec3f>(sphere_ids[i], 0);
    cv::Vec3f sq = tool.sphere_kalman_filters[tool_node_id].FilterData(
        squared_distance_raw);
    q.at<float>(i, 0) = sq[0];
    q.at<float>(i, 1) = sq[1];
    q.at<float>(i, 2) = sq[2];
    q_center += sq;
    ++tool_node_id;
  }
  p_center /= static_cast<float>(num_pts);
  q_center /= static_cast<float>(num_pts);

  // Subtract centroids
  cv::Mat p_r = p.clone(), q_r = q.clone();
  for (int r = 0; r < num_pts; ++r) {
    p_r.at<float>(r, 0) -= p_center[0];
    p_r.at<float>(r, 1) -= p_center[1];
    p_r.at<float>(r, 2) -= p_center[2];
    q_r.at<float>(r, 0) -= q_center[0];
    q_r.at<float>(r, 1) -= q_center[1];
    q_r.at<float>(r, 2) -= q_center[2];
  }

  // SVD of cross-covariance
  cv::Mat cov = p_r.t() * q_r;
  cv::SVD svd(cov);

  double d = cv::determinant(svd.vt.t() * svd.u.t());
  cv::Mat I = cv::Mat::eye(3, 3, CV_32F);
  I.at<float>(2, 2) = (d > 0) ? 1.f : -1.f;

  cv::Mat R = svd.vt.t() * I * svd.u.t();

  // Translation (still in mm)
  cv::Mat q_avg(3, 1, CV_32F), p_avg(3, 1, CV_32F);
  q_avg.at<float>(0, 0) = q_center[0];
  q_avg.at<float>(1, 0) = q_center[1];
  q_avg.at<float>(2, 0) = q_center[2];
  p_avg.at<float>(0, 0) = p_center[0];
  p_avg.at<float>(1, 0) = p_center[1];
  p_avg.at<float>(2, 0) = p_center[2];
  cv::Mat t = q_avg - R * p_avg;

  // Build 4×4 camera-space rigid-body transform (translation in metres)
  cv::Mat T_cam = cv::Mat::eye(4, 4, CV_32F);
  R.copyTo(T_cam(cv::Rect(0, 0, 3, 3)));
  T_cam.at<float>(0, 3) = t.at<float>(0, 0) / 1000.f;
  T_cam.at<float>(1, 3) = t.at<float>(1, 0) / 1000.f;
  T_cam.at<float>(2, 3) = t.at<float>(2, 0) / 1000.f;

  // Apply camera-to-world: T_world = camera_to_world_ * T_cam
  cv::Mat T_world = camera_to_world_ * T_cam;

  // Extract world-space position (metres)
  cv::Vec3f position(T_world.at<float>(0, 3), T_world.at<float>(1, 3),
                     T_world.at<float>(2, 3));

  // Extract world-space rotation matrix (upper-left 3×3 of T_world)
  cv::Mat R_world = T_world(cv::Rect(0, 0, 3, 3)).clone();

  /*
   * Convert world-space rotation matrix to quaternion using Shepperd's method.
   *
   * This is important here because small jitter in marker measurements can
   * amplify into orientation noise if quaternion extraction is fragile.
   */
  float tr = R_world.at<float>(0, 0) + R_world.at<float>(1, 1) +
             R_world.at<float>(2, 2);
  cv::Vec4f quat; // [qx, qy, qz, qw]
  quat[3] = std::sqrt(std::max(0.f, 1.f + tr)) / 2.f;
  quat[0] = std::sqrt(std::max(0.f, 1.f + R_world.at<float>(0, 0) -
                                        R_world.at<float>(1, 1) -
                                        R_world.at<float>(2, 2))) /
            2.f;
  quat[1] = std::sqrt(std::max(0.f, 1.f - R_world.at<float>(0, 0) +
                                        R_world.at<float>(1, 1) -
                                        R_world.at<float>(2, 2))) /
            2.f;
  quat[2] = std::sqrt(std::max(0.f, 1.f - R_world.at<float>(0, 0) -
                                        R_world.at<float>(1, 1) +
                                        R_world.at<float>(2, 2))) /
            2.f;
  quat[0] *=
      (R_world.at<float>(2, 1) - R_world.at<float>(1, 2)) >= 0.f ? 1.f : -1.f;
  quat[1] *=
      (R_world.at<float>(0, 2) - R_world.at<float>(2, 0)) >= 0.f ? 1.f : -1.f;
  quat[2] *=
      (R_world.at<float>(1, 0) - R_world.at<float>(0, 1)) >= 0.f ? 1.f : -1.f;

  // Low-pass filter: blend toward new position and slerp toward new rotation
  const cv::Mat &prev = tool.cur_transform;
  if (prev.at<float>(7, 0) == 1.f) { // only if previous frame was valid
    float alpha = tool.lowpass_factor_position;
    position =
        alpha * position + (1.f - alpha) * cv::Vec3f(prev.at<float>(0, 0),
                                                     prev.at<float>(1, 0),
                                                     prev.at<float>(2, 0));

    /*
     * Quaternion smoothing with Slerp (spherical linear interpolation).
     */
    cv::Vec4f q_old(prev.at<float>(3, 0), prev.at<float>(4, 0),
                    prev.at<float>(5, 0), prev.at<float>(6, 0));
    float dot = quat.dot(q_old);
    if (dot < 0.f) {
      q_old = -q_old;
      dot = -dot;
    } // ensure shortest path
    float t_lerp = tool.lowpass_factor_rotation;
    if (dot > 0.9995f) {
      // Nearly identical — linear interpolation is fine
      quat = (1.f - t_lerp) * q_old + t_lerp * quat;
    } else {
      float theta = std::acos(dot);
      float sin_th = std::sin(theta);
      quat = (std::sin((1.f - t_lerp) * theta) / sin_th) * q_old +
             (std::sin(t_lerp * theta) / sin_th) * quat;
    }
    // Normalize
    float qnorm = cv::norm(quat);
    if (qnorm > 1e-6f)
      quat /= qnorm;
  }

  cv::Mat out = cv::Mat::zeros(8, 1, CV_32F);
  out.at<float>(0, 0) = position[0];
  out.at<float>(1, 0) = position[1];
  out.at<float>(2, 0) = position[2];
  out.at<float>(3, 0) = quat[0]; // qx
  out.at<float>(4, 0) = quat[1]; // qy
  out.at<float>(5, 0) = quat[2]; // qz
  out.at<float>(6, 0) = quat[3]; // qw
  out.at<float>(7, 0) = 1.f;     // valid

  TOOL_TRK_LOG("MatchPointsKabsch '%s': world position=(%.3f, %.3f, %.3f), "
               "quat=(%.3f, %.3f, %.3f, %.3f), num_pts=%d, occluded=%zu",
               tool.identifier.c_str(), position[0], position[1], position[2],
               quat[0], quat[1], quat[2], quat[3], num_pts,
               occluded_nodes.size());
  return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetSecondsSinceTracked
// ─────────────────────────────────────────────────────────────────────────────
float ToolTracker::GetSecondsSinceTracked(const std::string &identifier) const {
  if (tool_index_.count(identifier) == 0)
    return -1.f;
  const TrackedTool &t = tools_.at(tool_index_.at(identifier));
  if (!t.ever_tracked)
    return -1.f;
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<float>(now - t.last_tracked_time).count();
}

} // namespace tool_tracking
} // namespace ml
