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

#pragma once

#include <opencv2/opencv.hpp>
#include <ml_depth_camera.h>
#include <vector>

// TODO: Decide on units - keep meters (ML2 native) or convert to mm (HoloLens2 convention)?
// TODO: Verify sphere_radius correction - should we add or subtract? Is 6mm correct?
// TODO: Determine intensity threshold values - need calibration with actual markers?

namespace ml {
namespace marker_detection {

/**
 * Configuration parameters for marker detection
 */
struct MarkerDetectionConfig {
    float intensity_threshold_min = 2000.0f;  // TODO: Calibrate with actual data
    float intensity_threshold_max = 3500.0f;
    float sphere_radius_mm = 5.0f;  // TODO: Verify physical marker size
    bool use_ambient_subtraction = false;  // Enable raw - ambient

    // Area filter: A_px ≈ π·r²·fx·fy/d²; accept if measured area in [min_ratio, max_ratio] * expected
    // TODO: does this actually make sense?
    float expected_area_min_ratio = 0.5f;
    float expected_area_max_ratio = 2.0f;
};

/**
 * Represents a detected IR marker with its 3D position and properties
 */
struct DetectedMarker {
    cv::Vec3f position_camera;  // 3D position in camera space (meters)
    cv::Point2f centroid_pixel; // Pixel coordinates of marker centroid
    int area_pixels;            // Area of marker blob in pixels
    float intensity;            // Average intensity value
    // All pixels belonging to this blob in image space (for visualization)
    std::vector<cv::Point2f> pixels;
};

/**
 * Represents a blob that was rejected by the marker classifier, used for
 * visualization/debugging (2D information only).
 */
struct RejectedBlob {
    cv::Point2f centroid_pixel; // Pixel coordinates of blob centroid
    int area_pixels;            // Area of blob in pixels
    float depth_m;              // Depth at centroid (meters), may be <= 0 for invalid depth
    // All pixels belonging to this blob in image space (for visualization)
    std::vector<cv::Point2f> pixels;
};

/**
 * Main marker detection class
 */
class MarkerDetection {
public:
    /**
     * Detect IR marker positions from depth camera data
     *
     * @param raw_intensity_buffer Raw intensity data from raw_depth_image
     * @param calibrated_depth_buffer Calibrated depth data in meters from depth_image
     * @param ambient_intensity_buffer Optional ambient raw data from ambient_raw_depth_image
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param intrinsics Camera intrinsic parameters
     * @param config Detection configuration parameters
     * @param rejected_blobs Optional output vector that will be filled with
     *        blobs rejected by the classifier (for visualization/debugging).
     * @return Vector of detected markers
     */
    static std::vector<DetectedMarker> detectMarkerPositions(
        float* raw_intensity_buffer,
        float* calibrated_depth_buffer,
        float* ambient_intensity_buffer,
        int width,
        int height,
        const MLDepthCameraIntrinsics& intrinsics,
        const MarkerDetectionConfig& config = MarkerDetectionConfig(),
        std::vector<RejectedBlob>* rejected_blobs = nullptr
    );

private:
    /**
     * Convert pixel coordinates to normalized camera plane coordinates
     *
     * @param u Pixel x-coordinate
     * @param v Pixel y-coordinate
     * @param intrinsics Camera intrinsic parameters
     * @param x Output normalized x in camera plane
     * @param y Output normalized y in camera plane
     */
    static void pixelToCameraPlane(
        float u, float v,
        const MLDepthCameraIntrinsics& intrinsics,
        float& x, float& y
    );

    /**
     * Expected marker area in pixels from A_px ≈ π·r²·fx·fy/d² (r,d in mm; fx,fy in pixels).
     * Used when use_distance_based_area is true to filter blobs by expected size at given depth.
     */
    static float expectedMarkerAreaPixels(
        float depth_m,
        float radius_mm,
        float focal_x_px,
        float focal_y_px
    );

    /**
     * Find marker blobs using connected components analysis
     *
     * @param binary_image Binary threshold image (8-bit)
     * @param intensity_image Original intensity image for centroid intensity lookup
     * @param calibrated_depth_buffer Calibrated depth data
     * @param width Image width
     * @param height Image height
     * @param intrinsics Camera intrinsic parameters
     * @param config Detection configuration
     * @param rejected_blobs Optional output vector that will be filled with
     *        blobs rejected by the classifier (for visualization/debugging).
     * @return Vector of detected markers
     */
    static std::vector<DetectedMarker> findMarkerBlobs(
        const cv::Mat& binary_image,
        const cv::Mat& intensity_image,
        float* calibrated_depth_buffer,
        int width,
        int height,
        const MLDepthCameraIntrinsics& intrinsics,
        const MarkerDetectionConfig& config,
        std::vector<RejectedBlob>* rejected_blobs
    );
};

} // namespace marker_detection
} // namespace ml
