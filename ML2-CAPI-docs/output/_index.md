# ML2 C++ SDK — Navigation-Relevant APIs

Reference documentation extracted from the Magic Leap 2 C++ SDK Doxygen XML.
Only the APIs relevant to porting the STTAR framework (HL2 IR marker tracking → ML2) are included.

## HL2 → ML2 API Mapping

| HL2 Concept | ML2 API | File |
|---|---|---|
| Research Mode AHAT sensor → MLDepthCamera (depth frames, intrinsics, pose) | [Depth Camera](depth_camera.md) |
| SpatialCoordinateSystem → MLPerception snapshots + coordinate frame transforms | [Perception](perception.md) |
| SpatialLocator → MLHeadTracking (head pose, tracking state) | [Head Tracking](head_tracking.md) |
| (alternative to IR) → MLCVCamera (CV camera pose for marker detection) | [CV Camera](cv_camera.md) |
| Camera intrinsics → MLCamera (general camera intrinsics, capture config) | [Camera](camera.md) |

## Modules

### [Depth Camera](depth_camera.md)
6 enums, 10 structs, 10 functions

### [Perception](perception.md)
2 enums, 4 structs, 13 functions

### [Head Tracking](head_tracking.md)
5 enums, 0 structs, 2 functions

### [CV Camera](cv_camera.md)
1 enums, 0 structs, 3 functions

### [Camera](camera.md)
13 enums, 13 structs, 25 functions
