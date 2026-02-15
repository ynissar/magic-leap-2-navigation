# CV Camera

## Enums

### MLCVCameraID
Camera id enum.
API Level: 5

| Value | Description |
|---|---|
| `MLCVCameraID_ColorCamera = 0` | Default camera id. |

## Functions

`MLResult MLCVCameraTrackingCreate(MLHandle  * out_handle)`
Create CV Camera Tracker.
API Level: 5
Permission: `android.permission.CAMERA (protection level: dangerous)`
- out_handle [out]: Tracker handle.
Returns:
- `MLResult_Ok` — On success.
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.
- `MLResult_PermissionDenied` — Necessary permission is missing.

`MLResult MLCVCameraGetFramePose(MLHandle camera_handle, MLHandle head_handle, MLCVCameraID id, MLTime camera_timestamp, MLTransform  * out_transform)`
Get the camera pose in the world coordinate system.
API Level: 5
- camera_handle [in]: MLHandle previously created with MLCVCameraTrackingCreate.
- head_handle [in]: MLHandle previously created with MLHeadTrackingCreate.
- id [in]: Camera id.
- camera_timestamp [in]: Time to request the pose.
- out_transform [out]: Transform from camera to world origin.
Returns:
- `MLResult_InvalidParam` — id parameter was not valid or out_transform parameter was not valid (null).
- `MLResult_Ok` — Obtained transform successfully.
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.
- `MLResult_PoseNotFound` — Pose is currently not available.
- `MLResult_UnspecifiedFailure` — Failed to obtain transform due to internal error.

`MLResult MLCVCameraTrackingDestroy(MLHandle camera_handle)`
Destroy Tracker after usage.
API Level: 5
- camera_handle [in]: MLHandle previously created with MLCVCameraTrackingCreate.
Returns:
- `MLResult_Ok` — On success.
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.
