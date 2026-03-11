#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "tool_definition.h"
#include <app_framework/toolset.h>
#include <algorithm>
#include <cmath>

namespace ml {
namespace tool_tracking {

// ─────────────────────────────────────────────────────────────────────────────
// ProcessFrame
// Entry point called once per depth camera frame after marker detection.
// Converts detected marker positions to mm, builds a pairwise distance frame
// map, runs the graph search for each registered tool, then resolves conflicts.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::ProcessFrame(
    const std::vector<ml::marker_detection::DetectedMarker>& markers)
{
    int num_frame_spheres = static_cast<int>(markers.size());
    if (num_frame_spheres < 3 || tools_.empty()) return;

    // Convert positions from metres to mm (all internal geometry is in mm)
    cv::Mat3f frame_spheres_mm(num_frame_spheres, 1);
    for (int i = 0; i < num_frame_spheres; ++i) {
        const cv::Vec3f& p = markers[i].position_camera;
        frame_spheres_mm.at<cv::Vec3f>(i, 0) = cv::Vec3f(p[0] * 1000.f,
                                                          p[1] * 1000.f,
                                                          p[2] * 1000.f);
    }

    // Build pairwise distance map for this frame
    cv::Mat frame_map(cv::Size(num_frame_spheres, num_frame_spheres), CV_32F, cv::Scalar(0.f));
    std::vector<Side> frame_sides;
    ConstructMap(frame_spheres_mm, num_frame_spheres, frame_map, frame_sides);

    // Run graph search for every registered tool
    int num_tools = static_cast<int>(tools_.size());
    std::vector<ToolResultContainer> raw_results(num_tools);
    for (int i = 0; i < num_tools; ++i) {
        raw_results[i].tool_id = i;
        if (!tools_[i].tracking_finished) continue;
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
void ToolTracker::TrackTool(TrackedTool& tool,
                             const cv::Mat3f& frame_spheres_mm,
                             const cv::Mat& frame_map,
                             const std::vector<Side>& frame_sides,
                             int num_frame_spheres,
                             ToolResultContainer& result) const
{
    tool.tracking_finished = false;

    if (num_frame_spheres < tool.min_visible_spheres) {
        tool.tracking_finished = true;
        return;
    }

    int max_occluded = tool.num_spheres - tool.min_visible_spheres;

    struct search_entry {
        std::vector<int> visited_nodes_frame;
        float combined_error{0.f};
        int   num_sides{0};
        std::vector<int> occluded_nodes_tool;
    };

    std::vector<search_entry> search_list;
    std::vector<Side> eligible_sides;
    std::vector<int>  hidden_nodes;

    // Seed: find frame sides whose length matches the first visible template side,
    // accounting for possible leading occlusions.
    for (int m = 0; m <= max_occluded; ++m) {
        std::vector<int> hidden_nodes_inside;
        for (int k = m + 1; k <= max_occluded + 1; ++k) {
            float cur_side_length = tool.map.at<float>(m, k);
            for (const Side& s : frame_sides) {
                if (std::abs(s.distance - cur_side_length) < tolerance_side_)
                    eligible_sides.push_back(s);
            }
            if (eligible_sides.empty() && max_occluded == 0) {
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
            tool.tracking_finished = true;
            return;
        }
        if (!eligible_sides.empty()) {
            std::vector<int> all_hidden;
            all_hidden.insert(all_hidden.end(), hidden_nodes.begin(), hidden_nodes.end());
            all_hidden.insert(all_hidden.end(), hidden_nodes_inside.begin(), hidden_nodes_inside.end());
            if (static_cast<int>(all_hidden.size()) <= max_occluded) {
                for (const Side& s : eligible_sides) {
                    search_list.push_back({{s.id_from, s.id_to},
                                           std::abs(s.distance - tool.map.at<float>(m, m + 1 + static_cast<int>(hidden_nodes_inside.size()))),
                                           1, all_hidden});
                    search_list.push_back({{s.id_to, s.id_from},
                                           std::abs(s.distance - tool.map.at<float>(m, m + 1 + static_cast<int>(hidden_nodes_inside.size()))),
                                           1, all_hidden});
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
        if (occluded <= max_occluded &&
            visited == (tool.num_spheres - occluded)) {
            if (curr.num_sides > 0 &&
                (curr.combined_error / curr.num_sides) < tolerance_avg_) {
                ToolResult r;
                r.error          = curr.combined_error;
                r.sphere_ids     = curr.visited_nodes_frame;
                r.occluded_nodes = curr.occluded_nodes_tool;
                result.candidates.push_back(r);
            }
            continue;
        }

        // Try to extend with each unvisited frame sphere
        for (int cand = 0; cand < num_frame_spheres; ++cand) {
            if (std::find(curr.visited_nodes_frame.begin(),
                          curr.visited_nodes_frame.end(), cand)
                != curr.visited_nodes_frame.end())
                continue;

            bool   exceeded = false;
            float  err_new  = 0.f;
            int    err_cnt  = 0;
            int    tnid     = 0; // tool node id, skipping occluded nodes

            for (int j = 0; j < visited; ++j) {
                while (std::find(curr.occluded_nodes_tool.begin(),
                                 curr.occluded_nodes_tool.end(), tnid)
                       != curr.occluded_nodes_tool.end())
                    ++tnid;

                int id1 = curr.visited_nodes_frame[j];
                int id2 = cand;
                if (id1 > id2) std::swap(id1, id2);

                // next tool node index = visited count + occlusion count
                float expected = tool.map.at<float>(tnid, visited + occluded);
                float err_side = std::abs(frame_map.at<float>(id1, id2) - expected);

                if (err_side > tolerance_side_) { exceeded = true; break; }
                err_new += err_side;
                ++err_cnt;
                ++tnid;
            }

            if (exceeded) {
                // Optionally mark next tool node as occluded and retry without cand
                if (occluded < max_occluded) {
                    std::vector<int> new_occl(curr.occluded_nodes_tool);
                    new_occl.push_back(visited + occluded);
                    search_list.push_back({curr.visited_nodes_frame,
                                           curr.combined_error,
                                           curr.num_sides, new_occl});
                }
                continue;
            }

            std::vector<int> new_visited(curr.visited_nodes_frame);
            new_visited.push_back(cand);
            search_list.push_back({new_visited,
                                   curr.combined_error + err_new,
                                   curr.num_sides + err_cnt,
                                   curr.occluded_nodes_tool});
        }
    }

    tool.tracking_finished = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// UnionSegmentation
// Greedy conflict resolution: sort all candidates by quality, take the best,
// then discard any other candidates that share a detected marker.
// Ported from IRToolTracker::UnionSegmentation.
// ─────────────────────────────────────────────────────────────────────────────
void ToolTracker::UnionSegmentation(ToolResultContainer* raw, int num_tools,
                                    const cv::Mat3f& frame_spheres_mm)
{
    std::vector<ToolResult> pool;
    for (int i = 0; i < num_tools; ++i) {
        for (ToolResult& r : raw[i].candidates) {
            r.tool_id = i;
            pool.push_back(r);
        }
    }
    std::sort(pool.begin(), pool.end(), &ToolResult::compare);

    while (!pool.empty()) {
        ToolResult best = pool.front();
        pool.erase(pool.begin());

        cv::Mat result = MatchPointsKabsch(tools_[best.tool_id], frame_spheres_mm,
                                           best.sphere_ids, best.occluded_nodes);
        if (result.at<float>(7, 0) == 1.f)
            tools_[best.tool_id].cur_transform = result.clone();

        // Remove candidates that share any detected sphere with the chosen result
        pool.erase(std::remove_if(pool.begin(), pool.end(),
            [&](const ToolResult& next) {
                if (next.tool_id == best.tool_id) return true;
                for (int cs : best.sphere_ids)
                    for (int ns : next.sphere_ids)
                        if (cs == ns) return true;
                return false;
            }), pool.end());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// MatchPointsKabsch
// Computes the 6DOF pose of a tool given the matched sphere IDs.
// Works entirely in mm, outputs position in metres.
// Ported from IRToolTracker::MatchPointsKabsch, adapted for ML2 (no world
// transform, no handedness flip, no Kalman filter).
// ─────────────────────────────────────────────────────────────────────────────
cv::Mat ToolTracker::MatchPointsKabsch(TrackedTool& tool,
                                        const cv::Mat3f& frame_spheres_mm,
                                        const std::vector<int>& sphere_ids,
                                        const std::vector<int>& occluded_nodes)
{
    cv::Mat zeros = cv::Mat::zeros(8, 1, CV_32F);
    int num_pts = tool.num_spheres - static_cast<int>(occluded_nodes.size());
    if (num_pts < 3) return zeros;

    cv::Mat p(num_pts, 3, CV_32F); // template points (mm)
    cv::Mat q(num_pts, 3, CV_32F); // detected points (mm)
    cv::Vec3f p_center(0.f), q_center(0.f);

    int tnid = 0;
    for (int i = 0; i < num_pts; ++i) {
        while (std::find(occluded_nodes.begin(), occluded_nodes.end(), tnid)
               != occluded_nodes.end())
            ++tnid;

        cv::Vec3f sp = tool.spheres_xyz_mm.at<cv::Vec3f>(tnid, 0);
        p.at<float>(i, 0) = sp[0]; p.at<float>(i, 1) = sp[1]; p.at<float>(i, 2) = sp[2];
        p_center += sp;

        // Apply per-sphere Kalman filter before feeding into Kabsch.
        // tnid tracks the tool-node index (skipping occluded nodes), matching
        // the sphere_kalman_filters vector layout exactly.
        cv::Vec3f sq_raw = frame_spheres_mm.at<cv::Vec3f>(sphere_ids[i], 0);
        cv::Vec3f sq     = tool.sphere_kalman_filters[tnid].FilterData(sq_raw);
        q.at<float>(i, 0) = sq[0]; q.at<float>(i, 1) = sq[1]; q.at<float>(i, 2) = sq[2];
        q_center += sq;
        ++tnid;
    }
    p_center /= static_cast<float>(num_pts);
    q_center /= static_cast<float>(num_pts);

    // Subtract centroids
    cv::Mat p_r = p.clone(), q_r = q.clone();
    for (int r = 0; r < num_pts; ++r) {
        p_r.at<float>(r, 0) -= p_center[0]; p_r.at<float>(r, 1) -= p_center[1]; p_r.at<float>(r, 2) -= p_center[2];
        q_r.at<float>(r, 0) -= q_center[0]; q_r.at<float>(r, 1) -= q_center[1]; q_r.at<float>(r, 2) -= q_center[2];
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
    q_avg.at<float>(0,0) = q_center[0]; q_avg.at<float>(1,0) = q_center[1]; q_avg.at<float>(2,0) = q_center[2];
    p_avg.at<float>(0,0) = p_center[0]; p_avg.at<float>(1,0) = p_center[1]; p_avg.at<float>(2,0) = p_center[2];
    cv::Mat t = q_avg - R * p_avg;

    // Convert position to metres
    cv::Vec3f position(t.at<float>(0,0) / 1000.f,
                       t.at<float>(1,0) / 1000.f,
                       t.at<float>(2,0) / 1000.f);

    // Extract quaternion from rotation matrix (Shepperd's method)
    float tr = R.at<float>(0,0) + R.at<float>(1,1) + R.at<float>(2,2);
    cv::Vec4f quat; // [qx, qy, qz, qw]
    quat[3] = std::sqrt(std::max(0.f, 1.f + tr)) / 2.f;
    quat[0] = std::sqrt(std::max(0.f, 1.f + R.at<float>(0,0) - R.at<float>(1,1) - R.at<float>(2,2))) / 2.f;
    quat[1] = std::sqrt(std::max(0.f, 1.f - R.at<float>(0,0) + R.at<float>(1,1) - R.at<float>(2,2))) / 2.f;
    quat[2] = std::sqrt(std::max(0.f, 1.f - R.at<float>(0,0) - R.at<float>(1,1) + R.at<float>(2,2))) / 2.f;
    quat[0] *= (R.at<float>(2,1) - R.at<float>(1,2)) >= 0.f ? 1.f : -1.f;
    quat[1] *= (R.at<float>(0,2) - R.at<float>(2,0)) >= 0.f ? 1.f : -1.f;
    quat[2] *= (R.at<float>(1,0) - R.at<float>(0,1)) >= 0.f ? 1.f : -1.f;

    // Low-pass filter: blend toward new position and slerp toward new rotation
    const cv::Mat& prev = tool.cur_transform;
    if (prev.at<float>(7, 0) == 1.f) { // only if previous frame was valid
        float alpha = tool.lowpass_factor_position;
        position = alpha * position + (1.f - alpha) *
                   cv::Vec3f(prev.at<float>(0,0), prev.at<float>(1,0), prev.at<float>(2,0));

        // Slerp between previous and new quaternion
        cv::Vec4f q_old(prev.at<float>(3,0), prev.at<float>(4,0),
                        prev.at<float>(5,0), prev.at<float>(6,0));
        float dot = quat.dot(q_old);
        if (dot < 0.f) { q_old = -q_old; dot = -dot; } // ensure shortest path
        float t_lerp = tool.lowpass_factor_rotation;
        if (dot > 0.9995f) {
            // Nearly identical — linear interpolation is fine
            quat = (1.f - t_lerp) * q_old + t_lerp * quat;
        } else {
            float theta   = std::acos(dot);
            float sin_th  = std::sin(theta);
            quat = (std::sin((1.f - t_lerp) * theta) / sin_th) * q_old
                 + (std::sin(t_lerp * theta)          / sin_th) * quat;
        }
        // Normalize
        float qnorm = cv::norm(quat);
        if (qnorm > 1e-6f) quat /= qnorm;
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
    return out;
}

} // namespace tool_tracking
} // namespace ml
