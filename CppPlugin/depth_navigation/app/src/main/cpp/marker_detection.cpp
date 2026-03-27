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
#include <android/log.h>

#define MARKER_DETECTION_LOG(...) \
    __android_log_print(ANDROID_LOG_INFO, "MarkerDetection", __VA_ARGS__)

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
    const MarkerDetectionConfig& config,
    std::vector<RejectedBlob>* rejected_blobs)
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
        MARKER_DETECTION_LOG("[DEBUG] Using ambient subtraction for marker detection");
    } else {
        intensity_image = raw_intensity;
    }

    // Log intensity statistics
    double min_val, max_val;
    cv::minMaxLoc(intensity_image, &min_val, &max_val);
    MARKER_DETECTION_LOG("[DEBUG] Intensity range: min=%.2f, max=%.2f", min_val, max_val);

    // Threshold to find bright spots
    // First, apply a small Gaussian blur so that nearby high-intensity pixels
    // merge into a smoother spot before thresholding.
    cv::Mat blurred_intensity;
    int gk = std::max(1, config.gaussian_blur_kernel_size | 1); // ensure odd, >= 1
    if (gk > 1) {
        cv::GaussianBlur(intensity_image, blurred_intensity, cv::Size(gk, gk), 0);
    } else {
        blurred_intensity = intensity_image;
    }

    cv::Mat binary_mask;
    cv::inRange(blurred_intensity, config.intensity_threshold_min,
                config.intensity_threshold_max, binary_mask);

    int bright_pixels = cv::countNonZero(binary_mask);
    MARKER_DETECTION_LOG(
        "[DEBUG] Bright pixels after threshold: %d (%.2f%%)",
        bright_pixels,
        100.0 * bright_pixels / (width * height));

    // Convert to 8-bit for connectedComponents (it requires 8-bit input)
    cv::Mat binary_8bit;
    binary_mask.convertTo(binary_8bit, CV_8UC1);

    // Apply a small morphological closing to merge nearby bright pixels
    // This helps when a single physical marker appears as several tiny islands.
    {
        int ks = std::max(1, config.morphology_kernel_size | 1); // ensure odd, >= 1
        cv::Mat element = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, cv::Size(ks, ks));
        cv::morphologyEx(binary_8bit, binary_8bit, cv::MORPH_CLOSE, element);
        int bright_pixels_after = cv::countNonZero(binary_8bit);
        MARKER_DETECTION_LOG("[DEBUG] Bright pixels after morphology: %d", bright_pixels_after);
    }

    // Find blobs and convert to 3D
    return findMarkerBlobs(binary_8bit, intensity_image,
                          calibrated_depth_buffer, width, height,
                          intrinsics, config, rejected_blobs);
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

    if (depth_m <= 0.f) {
        return 0.f;
    }
    float d_mm = depth_m * 1000.f;
    float d_cm = depth_m * 100.f;
    MARKER_DETECTION_LOG("[DEBUG] Depth in m and cm: depth_m=%.3f, d_cm=%.3f", depth_m, d_cm);
    float d2_mm2 = d_mm * d_mm;
    float r2_mm2 = radius_mm * radius_mm;
    float area = static_cast<float>(M_PI) * r2_mm2 * focal_x_px * focal_y_px / d2_mm2;
    MARKER_DETECTION_LOG("[DEBUG] Expected marker area pixels: area=%.3f", area);
    return area;
}

std::vector<DetectedMarker> MarkerDetection::findMarkerBlobs(
    const cv::Mat& binary_image,
    const cv::Mat& intensity_image,
    float* calibrated_depth_buffer,
    int width,
    int height,
    const MLDepthCameraIntrinsics& intrinsics,
    const MarkerDetectionConfig& config,
    std::vector<RejectedBlob>* rejected_blobs)
{
    std::vector<DetectedMarker> markers;

    cv::Mat labels, stats, centroids;
    int num_components = cv::connectedComponentsWithStats(
        binary_image, labels, stats, centroids, 8);

    MARKER_DETECTION_LOG("[DEBUG] Found %d connected components", num_components - 1);

    // Start from i=1 to skip the background component (label 0)
    for (int i = 1; i < num_components; i++) {
        int area = stats.at<int32_t>(i, cv::CC_STAT_AREA);

        // Collect all pixels that belong to this connected component.
        // We use the bounding box from stats to limit the search region.
        int left   = stats.at<int32_t>(i, cv::CC_STAT_LEFT);
        int top    = stats.at<int32_t>(i, cv::CC_STAT_TOP);
        int widthB = stats.at<int32_t>(i, cv::CC_STAT_WIDTH);
        int heightB= stats.at<int32_t>(i, cv::CC_STAT_HEIGHT);

        int right  = left + widthB;
        int bottom = top + heightB;

        std::vector<cv::Point2f> blob_pixels;
        blob_pixels.reserve(area);
        for (int y = top; y < bottom; ++y) {
            const int32_t* label_row = labels.ptr<int32_t>(y);
            for (int x = left; x < right; ++x) {
                if (label_row[x] == i) {
                    blob_pixels.emplace_back(static_cast<float>(x),
                                             static_cast<float>(y));
                }
            }
        }

        // Get centroid pixel coordinates first (needed for depth lookup and distance-based area)
        double u = centroids.at<double>(i, 0);
        double v = centroids.at<double>(i, 1);

        // Bounds check
        if (u < 0 || u >= width || v < 0 || v >= height) {
            MARKER_DETECTION_LOG(
                "[DEBUG] Rejected blob %d: centroid out of bounds (%.1f, %.1f)",
                i,
                u,
                v);
            if (rejected_blobs) {
                RejectedBlob blob;
                blob.centroid_pixel = cv::Point2f(static_cast<float>(u), static_cast<float>(v));
                blob.area_pixels = area;
                blob.depth_m = 0.0f;
                blob.intensity = 0.0f;
                blob.expected_area = 0.0f;
                blob.expected_area_min = 0.0f;
                blob.expected_area_max = 0.0f;
                blob.reason = RejectionReason::OutOfBounds;
                blob.pixels = std::move(blob_pixels);
                rejected_blobs->push_back(blob);
            }
            continue;
        }

        // Lookup calibrated depth (in meters)
        int pixel_idx = static_cast<int>(v) * width + static_cast<int>(u);
        float depth_m = calibrated_depth_buffer[pixel_idx];

        // Skip invalid depth values
        if (depth_m <= 0.0f || std::isnan(depth_m) || std::isinf(depth_m)) {
            MARKER_DETECTION_LOG(
                "[DEBUG] Rejected blob %d: invalid depth=%.3f",
                i,
                depth_m);
            if (rejected_blobs) {
                RejectedBlob blob;
                blob.centroid_pixel = cv::Point2f(static_cast<float>(u), static_cast<float>(v));
                blob.area_pixels = area;
                blob.depth_m = depth_m;
                blob.intensity = intensity_image.at<float>(static_cast<int>(v), static_cast<int>(u));
                blob.expected_area = 0.0f;
                blob.expected_area_min = 0.0f;
                blob.expected_area_max = 0.0f;
                blob.reason = RejectionReason::InvalidDepth;
                blob.pixels = std::move(blob_pixels);
                rejected_blobs->push_back(blob);
            }
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
            MARKER_DETECTION_LOG(
                "[DEBUG] Rejected blob %d: area=%d (expected %.1f, range [%.1f, %.1f]), depth=%.3fm",
                i,
                area,
                expected,
                min_expected,
                max_expected,
                depth_m);
            if (rejected_blobs) {
                RejectedBlob blob;
                blob.centroid_pixel = cv::Point2f(static_cast<float>(u), static_cast<float>(v));
                blob.area_pixels = area;
                blob.depth_m = depth_m;
                blob.intensity = intensity_image.at<float>(static_cast<int>(v), static_cast<int>(u));
                blob.expected_area = expected;
                blob.expected_area_min = min_expected;
                blob.expected_area_max = max_expected;
                blob.reason = RejectionReason::AreaMismatch;
                blob.pixels = std::move(blob_pixels);
                rejected_blobs->push_back(blob);
            }
            continue;
        }

        MARKER_DETECTION_LOG(
            "[DEBUG] Blob %d: centroid=(%.1f,%.1f) depth=%.3fm area=%d (expected %.1f, range [%.1f, %.1f]), depth= %.3fm",
            i,
            u,
            v,
            depth_m,
            area,
            expected,
            min_expected,
            max_expected,
            depth_m);

        // Convert pixel to camera plane
        float x, y;
        pixelToCameraPlane(static_cast<float>(u), static_cast<float>(v),
                          intrinsics, x, y);

        // TODO: Apply sphere radius correction
        // The marker is a sphere, so the center is offset from the surface
        float depth_corrected = depth_m + (config.sphere_radius_mm / 1000.0f);
        MARKER_DETECTION_LOG(
            "[DEBUG] Depth before correction: %.3f, after correction: %.3f",
            depth_m,
            depth_corrected);
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
        marker.pixels = std::move(blob_pixels);

        markers.push_back(marker);
    }

    return markers;
}

} // namespace marker_detection
} // namespace ml
