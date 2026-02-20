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

#include "marker_detection.h"
#include <app_framework/toolset.h>
#include <cmath>

// Uncomment to enable debug logging
#define DEBUG_MODE

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ml {
namespace marker_detection {

std::vector<DetectedMarker> MarkerDetection::detectMarkerPositions(
    float* raw_intensity_buffer,
    float* calibrated_depth_buffer,
    float* ambient_intensity_buffer,
    int width,
    int height,
    const MLDepthCameraIntrinsics& intrinsics,
    const MarkerDetectionConfig& config)
{
    // Convert to cv::Mat for easier processing
    cv::Mat raw_intensity(height, width, CV_32FC1, raw_intensity_buffer);

    // Optional: Subtract ambient for better SNR
    cv::Mat intensity_image;
    if (config.use_ambient_subtraction && ambient_intensity_buffer != nullptr) {
        cv::Mat ambient(height, width, CV_32FC1, ambient_intensity_buffer);
        intensity_image = raw_intensity - ambient;
        // Clamp negative values to zero
        cv::threshold(intensity_image, intensity_image, 0, 0, cv::THRESH_TOZERO);
#ifdef DEBUG_MODE
        ALOGI("[DEBUG] Using ambient subtraction for marker detection");
#endif
    } else {
        intensity_image = raw_intensity;
    }

#ifdef DEBUG_MODE
    // Log intensity statistics
    double min_val, max_val;
    cv::minMaxLoc(intensity_image, &min_val, &max_val);
    ALOGI("[DEBUG] Intensity range: min=%.2f, max=%.2f", min_val, max_val);
#endif

    // Threshold to find bright spots
    cv::Mat binary_mask;
    cv::inRange(intensity_image, config.intensity_threshold_min,
                config.intensity_threshold_max, binary_mask);

#ifdef DEBUG_MODE
    int bright_pixels = cv::countNonZero(binary_mask);
    ALOGI("[DEBUG] Bright pixels after threshold: %d (%.2f%%)",
          bright_pixels, 100.0 * bright_pixels / (width * height));
#endif

    // Convert to 8-bit for connectedComponents (it requires 8-bit input)
    cv::Mat binary_8bit;
    binary_mask.convertTo(binary_8bit, CV_8UC1);

    // Find blobs and convert to 3D
    return findMarkerBlobs(binary_8bit, intensity_image,
                          calibrated_depth_buffer, width, height,
                          intrinsics, config);
}

void MarkerDetection::pixelToCameraPlane(
    float u, float v,
    const MLDepthCameraIntrinsics& intrinsics,
    float& x, float& y)
{
    // Build camera matrix from intrinsics
    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) <<
        intrinsics.focal_length.x, 0.0, intrinsics.principal_point.x,
        0.0, intrinsics.focal_length.y, intrinsics.principal_point.y,
        0.0, 0.0, 1.0);

    // Distortion coefficients [k1, k2, p1, p2, k3]
    cv::Mat dist_coeffs = (cv::Mat_<double>(5, 1) <<
        intrinsics.distortion[0],  // k1
        intrinsics.distortion[1],  // k2
        intrinsics.distortion[2],  // p1
        intrinsics.distortion[3],  // p2
        intrinsics.distortion[4]); // k3

    // Input pixel point
    std::vector<cv::Point2f> input_points = {cv::Point2f(u, v)};
    std::vector<cv::Point2f> undistorted_points;

    // Undistort the point and get normalized camera plane coordinates
    cv::undistortPoints(input_points, undistorted_points, camera_matrix, dist_coeffs);

    // Extract normalized coordinates (already in camera plane)
    x = undistorted_points[0].x;
    y = undistorted_points[0].y;
}

float MarkerDetection::expectedMarkerAreaPixels(
    float depth_m,
    float radius_mm,
    float focal_x_px,
    float focal_y_px)
{
    // A_px ≈ π·r²·fx·fy / d²
    // r, d in mm; fx, fy in pixels (intrinsics). depth_m in meters -> d_mm = depth_m * 1000
    if (depth_m <= 0.f) return 0.f;
    float d_mm = depth_m * 1000.f;
    float d2_mm2 = d_mm * d_mm;
    float r2_mm2 = radius_mm * radius_mm;
    return static_cast<float>(M_PI) * r2_mm2 * focal_x_px * focal_y_px / d2_mm2;
}

std::vector<DetectedMarker> MarkerDetection::findMarkerBlobs(
    const cv::Mat& binary_image,
    const cv::Mat& intensity_image,
    float* calibrated_depth_buffer,
    int width,
    int height,
    const MLDepthCameraIntrinsics& intrinsics,
    const MarkerDetectionConfig& config)
{
    std::vector<DetectedMarker> markers;

    cv::Mat labels, stats, centroids;
    int num_components = cv::connectedComponentsWithStats(
        binary_image, labels, stats, centroids, 8);

#ifdef DEBUG_MODE
    ALOGI("[DEBUG] Found %d connected components", num_components - 1);
#endif

    // Start from i=1 to skip the background component (label 0)
    for (int i = 1; i < num_components; i++) {
        int area = stats.at<int32_t>(i, cv::CC_STAT_AREA);

        // Get centroid pixel coordinates first (needed for depth lookup and distance-based area)
        double u = centroids.at<double>(i, 0);
        double v = centroids.at<double>(i, 1);

        // Bounds check
        if (u < 0 || u >= width || v < 0 || v >= height) {
#ifdef DEBUG_MODE
            ALOGI("[DEBUG] Rejected blob %d: centroid out of bounds (%.1f, %.1f)", i, u, v);
#endif
            continue;
        }

        // Lookup calibrated depth (in meters)
        int pixel_idx = static_cast<int>(v) * width + static_cast<int>(u);
        float depth_m = calibrated_depth_buffer[pixel_idx];

        // Skip invalid depth values
        if (depth_m <= 0.0f || std::isnan(depth_m) || std::isinf(depth_m)) {
#ifdef DEBUG_MODE
            ALOGI("[DEBUG] Rejected blob %d: invalid depth=%.3f", i, depth_m);
#endif
            continue;
        }

        // Filter by blob area: A_px ≈ π·r²·fx·fy/d² (expected area from distance and radius)
        float expected = expectedMarkerAreaPixels(
            depth_m,
            config.sphere_radius_mm,
            intrinsics.focal_length.x,
            intrinsics.focal_length.y);
        float min_expected = config.expected_area_min_ratio * expected;
        float max_expected = config.expected_area_max_ratio * expected;
        if (expected <= 0.f || area < min_expected || area > max_expected) {
#ifdef DEBUG_MODE
            ALOGI("[DEBUG] Rejected blob %d: area=%d (expected %.1f, range [%.1f, %.1f])",
                  i, area, expected, min_expected, max_expected);
#endif
            continue;
        }

#ifdef DEBUG_MODE
        ALOGI("[DEBUG] Blob %d: centroid=(%.1f,%.1f) depth=%.3fm area=%d",
              i, u, v, depth_m, area);
#endif

        // Convert pixel to camera plane
        float x, y;
        pixelToCameraPlane(static_cast<float>(u), static_cast<float>(v),
                          intrinsics, x, y);

        // TODO: Apply sphere radius correction
        // The marker is a sphere, so the center is offset from the surface
        float depth_corrected = depth_m + (config.sphere_radius_mm / 1000.0f);
        // float depth_corrected = depth_m;  // Using raw depth for now

        // Compute 3D position in camera space
        // The direction vector from camera center through the pixel
        cv::Vec3f direction(x, y, 1.0f);
        float norm = cv::norm(direction);
        cv::Vec3f position_3d = (direction / norm) * depth_corrected;

        // Get intensity at centroid
        float centroid_intensity = intensity_image.at<float>(static_cast<int>(v),
                                                             static_cast<int>(u));

        // Create marker
        DetectedMarker marker;
        marker.position_camera = position_3d;  // In meters
        marker.centroid_pixel = cv::Point2f(static_cast<float>(u), static_cast<float>(v));
        marker.area_pixels = area;
        marker.intensity = centroid_intensity;

        markers.push_back(marker);
    }

    return markers;
}

} // namespace marker_detection
} // namespace ml
