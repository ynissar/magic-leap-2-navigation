#pragma once

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>

namespace ml {
namespace tool_tracking {

// Per-sphere 6-state Kalman filter (position + velocity).
// Ported from HoloLens2-IRTracking-main/IRKalmanFilter.h into the
// ml::tool_tracking namespace. All coordinates are in mm (matching the internal
// geometry convention).
//
// State:       [x, y, z, vx, vy, vz]  (constant-velocity model)
// Measurement: [x, y, z]              (position only)
//
// Default noise values match the HL2 reference:
//   positionNoise    = 1e-4  (prediction is usually within 0.1 mm of position)
//   velocityNoise    = 3.0   (velocity is loosely constrained)
//   measurementNoise = 1.0   (measurements are usually within ~3 mm of true
//   position)
class ToolKalmanFilter {
public:
  ToolKalmanFilter() : ToolKalmanFilter(1.f, 1e-4f, 3.f) {}

  ToolKalmanFilter(float measurementNoise, float positionNoise,
                   float velocityNoise)
      : measurement_noise_(measurementNoise), position_noise_(positionNoise),
        velocity_noise_(velocityNoise) {
    filter_ = cv::KalmanFilter(6, 3, 0, CV_32F);
  }

  // Reset the filter with new noise parameters.
  // The filter will re-initialise lazily on the next FilterData() call.
  void Reset(float measurementNoise, float positionNoise, float velocityNoise) {
    measurement_noise_ = measurementNoise;
    position_noise_ = positionNoise;
    velocity_noise_ = velocityNoise;
    initialized_ = false;
    filter_ = cv::KalmanFilter(6, 3, 0, CV_32F);
  }

  // Predict → correct with the given measurement.
  // On the very first call the filter is lazily initialised to the observed
  // position.
  cv::Vec3f FilterData(cv::Vec3f value) {
    if (!initialized_) {
      InitializeFilter(value);
    }
    cv::Mat prediction = filter_.predict();
    cv::Mat meas = cv::Mat(value).reshape(1, 3);
    cv::Mat correction = filter_.correct(meas);
    return cv::Vec3f(correction.at<float>(0, 0), correction.at<float>(1, 0),
                     correction.at<float>(2, 0));
  }

private:
  void InitializeFilter(cv::Vec3f value) {
    // State-transition matrix: constant-velocity model (dt == 1 frame)
    cv::Mat A = cv::Mat::eye(6, 6, CV_32F);
    A.at<float>(0, 3) = 1.f; // x  += vx
    A.at<float>(1, 4) = 1.f; // y  += vy
    A.at<float>(2, 5) = 1.f; // z  += vz
    filter_.transitionMatrix = A;

    // Measurement matrix: observe [x, y, z] from state
    cv::Mat H = cv::Mat::zeros(3, 6, CV_32F);
    H.at<float>(0, 0) = 1.f;
    H.at<float>(1, 1) = 1.f;
    H.at<float>(2, 2) = 1.f;
    filter_.measurementMatrix = H;

    // Process noise (Q): position and velocity components
    cv::Mat Q = cv::Mat::zeros(6, 6, CV_32F);
    Q.at<float>(0, 0) = Q.at<float>(1, 1) = Q.at<float>(2, 2) = position_noise_;
    Q.at<float>(3, 3) = Q.at<float>(4, 4) = Q.at<float>(5, 5) = velocity_noise_;
    filter_.processNoiseCov = Q;

    // Measurement noise (R)
    filter_.measurementNoiseCov =
        cv::Mat::eye(3, 3, CV_32F) * measurement_noise_;

    // Initial state: seed position from first observation, velocity = 0
    cv::Mat x = cv::Mat::zeros(6, 1, CV_32F);
    x.at<float>(0, 0) = value[0];
    x.at<float>(1, 0) = value[1];
    x.at<float>(2, 0) = value[2];
    filter_.statePre = x;
    filter_.statePost = x.clone();

    // Initial error covariance
    filter_.errorCovPost = cv::Mat::eye(6, 6, CV_32F);

    initialized_ = true;
  }

  cv::KalmanFilter filter_;
  bool initialized_{false};
  float measurement_noise_;
  float position_noise_;
  float velocity_noise_;
};

} // namespace tool_tracking
} // namespace ml
