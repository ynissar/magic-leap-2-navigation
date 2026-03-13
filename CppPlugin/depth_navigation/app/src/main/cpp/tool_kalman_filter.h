#pragma once

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>

namespace ml {
namespace tool_tracking {

// Per-sphere 6-state Kalman filter (position + velocity).
// Ported from HoloLens2-IRTracking-main/IRKalmanFilter.h into the ml::tool_tracking
// namespace. All coordinates are in mm (matching the internal geometry convention).
//
// State:       [x, y, z, vx, vy, vz]  (constant-velocity model)
// Measurement: [x, y, z]              (position only)
//
// Default noise values match the HL2 reference:
//   positionNoise    = 1e-4  (prediction is usually within 0.1 mm of position)
//   velocityNoise    = 3.0   (velocity is loosely constrained)
//   measurementNoise = 1.0   (measurements are usually within ~3 mm of true position)
class ToolKalmanFilter {
public:
    ToolKalmanFilter()
        : ToolKalmanFilter(1.f, 1e-4f, 3.f) {}

    ToolKalmanFilter(float measurementNoise, float positionNoise, float velocityNoise)
        : m_fMeasurementNoise(measurementNoise),
          m_fPositionNoise(positionNoise),
          m_fVelocityNoise(velocityNoise) {
        m_filter = cv::KalmanFilter(6, 3, 0, CV_32F);
    }

    // Reset the filter with new noise parameters.
    // The filter will re-initialise lazily on the next FilterData() call.
    void Reset(float measurementNoise, float positionNoise, float velocityNoise) {
        m_fMeasurementNoise = measurementNoise;
        m_fPositionNoise    = positionNoise;
        m_fVelocityNoise    = velocityNoise;
        m_bInitialized      = false;
        m_filter = cv::KalmanFilter(6, 3, 0, CV_32F);
    }

    // Predict → correct with the given measurement.
    // On the very first call the filter is lazily initialised to the observed position.
    cv::Vec3f FilterData(cv::Vec3f value) {
        if (!m_bInitialized) {
            InitializeFilter(value);
        }
        cv::Mat prediction = m_filter.predict();
        cv::Mat meas = cv::Mat(value).reshape(1, 3);
        cv::Mat correction = m_filter.correct(meas);
        return cv::Vec3f(correction.at<float>(0, 0),
                         correction.at<float>(1, 0),
                         correction.at<float>(2, 0));
    }

private:
    void InitializeFilter(cv::Vec3f value) {
        // State-transition matrix: constant-velocity model (dt == 1 frame)
        cv::Mat A = cv::Mat::eye(6, 6, CV_32F);
        A.at<float>(0, 3) = 1.f; // x  += vx
        A.at<float>(1, 4) = 1.f; // y  += vy
        A.at<float>(2, 5) = 1.f; // z  += vz
        m_filter.transitionMatrix = A;

        // Measurement matrix: observe [x, y, z] from state
        cv::Mat H = cv::Mat::zeros(3, 6, CV_32F);
        H.at<float>(0, 0) = 1.f;
        H.at<float>(1, 1) = 1.f;
        H.at<float>(2, 2) = 1.f;
        m_filter.measurementMatrix = H;

        // Process noise (Q): position and velocity components
        cv::Mat Q = cv::Mat::zeros(6, 6, CV_32F);
        Q.at<float>(0, 0) = Q.at<float>(1, 1) = Q.at<float>(2, 2) = m_fPositionNoise;
        Q.at<float>(3, 3) = Q.at<float>(4, 4) = Q.at<float>(5, 5) = m_fVelocityNoise;
        m_filter.processNoiseCov = Q;

        // Measurement noise (R)
        m_filter.measurementNoiseCov = cv::Mat::eye(3, 3, CV_32F) * m_fMeasurementNoise;

        // Initial state: seed position from first observation, velocity = 0
        cv::Mat x = cv::Mat::zeros(6, 1, CV_32F);
        x.at<float>(0, 0) = value[0];
        x.at<float>(1, 0) = value[1];
        x.at<float>(2, 0) = value[2];
        m_filter.statePre  = x;
        m_filter.statePost = x.clone();

        // Initial error covariance
        m_filter.errorCovPost = cv::Mat::eye(6, 6, CV_32F);

        m_bInitialized = true;
    }

    cv::KalmanFilter m_filter;
    bool  m_bInitialized{false};
    float m_fMeasurementNoise;
    float m_fPositionNoise;
    float m_fVelocityNoise;
};

} // namespace tool_tracking
} // namespace ml
