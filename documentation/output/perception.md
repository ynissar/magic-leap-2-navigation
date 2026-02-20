# Perception

## Enums

### (constants)

| Value | Description |
|---|---|
| `MLResultAPIPrefix_Snapshot = ( 0x87b8  << 16)` | Defines the prefix for MLSnapshotResult codes. |

### MLSnapshotResult
Return values for Snapshot API calls.

| Value | Description |
|---|---|
| `MLSnapshotResult_DerivativesNotCalculated = MLResultAPIPrefix_Snapshot` | Derivatives not calculated for requested coordinate frame. |

## Structs

### MLCoordinateFrameUID
A unique identifier which represents a coordinate frame.

- `uint64_t data[2]`

### MLPerceptionSettings
Settings for initializing the perception system.

- `uint16_t override_port`

### MLTransformDerivatives
Velocity and acceleration derivatives for a related transform.
API Level: 8

- `uint32_t version` — Version of this structure.
- `MLVec3f linear_velocity_m_s` — Linear velocity in meters per second, expressed in destination frame coordinates.
- `MLVec3f linear_acceleration_m_s2` — Linear acceleration in meters per second squared, expressed in destination frame coordinates.
- `MLVec3f angular_velocity_r_s` — Angular velocity in radians per second, expressed in destination frame coordinates.
- `MLVec3f angular_acceleration_r_s2` — Angular acceleration in radians per second squared, expressed in destination frame coordinates.

### MLSnapshotStaticData
Static information about the snapshot system.
API Level: 30

- `uint32_t version` — Version of this structure.
- `MLCoordinateFrameUID coord_world_origin` — Coordinate frame ID.

## Functions

`MLResult MLPerceptionInitSettings(MLPerceptionSettings  * out_settings)`
Initializes the perception system with the passed in settings.
- out_settings [out]: Initializes the perception system with these settings.
Returns:
- `MLResult_InvalidParam` — Failed to initialize the perception settings due to an invalid input parameter.
- `MLResult_Ok` — Successfully initialized the perception settings.
- `MLResult_UnspecifiedFailure` — Failed to initialize the perception settings due to an unknown error.

`MLResult MLPerceptionStartup(MLPerceptionSettings  * settings)`
Starts the perception system.
- settings [in]: The perception system starts with these settings.
Returns:
- `MLResult_Ok` — Successfully started perception system.
- `MLResult_UnspecifiedFailure` — Failed to start perception system due to an unknown failure.

`MLResult MLPerceptionShutdown()`
Shuts down the perception system and cleans up all resources used by it.
Returns:
- `MLResult_Ok` — Successfully shut down the perception system.
- `MLResult_UnspecifiedFailure` — Failed to shut down the perception system because of an uknown failure.

`MLResult MLPerceptionGetSnapshot(MLSnapshot  ** out_snapshot)`
Pulls in the latest state of all persistent transforms and all enabled trackers extrapolated to the next frame time.
- out_snapshot [out]: Pointer to a pointer containing an  MLSnapshot  on success.
Returns:
- `MLResult_Ok` — Successfully created snapshot.
- `MLResult_InvalidParam` — out_snapshot parameter was not valid (null).
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.

`MLResult MLPerceptionGetPredictedSnapshot(MLTime timestamp, MLSnapshot  ** out_snapshot)`
Pulls in the state of all persistent transforms and all enabled trackers extrapolated to the provided timestamp.
API Level: 27
- timestamp [in]: Timestamp representing the time for which to predict poses.
- out_snapshot [out]: Pointer to a pointer containing an  MLSnapshot  on success.
Returns:
- `MLResult_Ok` — Successfully created snapshot.
- `MLResult_InvalidTimestamp` — Timestamp is either more than 100ms in the future or too old for cached state.
- `MLResult_InvalidParam` — Output parameter was not valid (null).
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.

`MLResult MLPerceptionReleaseSnapshot(MLSnapshot  * snap)`
Releases specified  #MLSnapshot  object.
- snap [in]: Pointer to a valid snap object.
Returns:
- `MLResult_Ok` — Successfully released snapshot.
- `MLResult_InvalidParam` — snapshot parameter was not valid (null).
- `MLResult_PerceptionSystemNotStarted` — Perception System has not been started.

`void MLTransformDerivativesInit(MLTransformDerivatives  * inout_data)`
Intializes the default values for  MLTransformDerivatives .
API Level: 8
- inout_data [inout]: The object that will be initialized with default values.

`void MLSnapshotStaticDataInit(MLSnapshotStaticData  * inout_data)`
Intializes the default values for  MLSnapshotStaticData .
API Level: 30
- inout_data [inout]: The object that will be initialized with default values.

`MLResult MLSnapshotGetStaticData(MLSnapshotStaticData  * out_static_data)`
Get the static data pertaining to the snapshot system.
API Level: 30
- out_static_data [out]: Target to populate the data about snapshot system.
Returns:
- `MLResult_InvalidParam` — Failed to obtain static data due to invalid parameter.
- `MLResult_Ok` — Obtained static data successfully.
- `MLResult_UnspecifiedFailure` — Failed to obtain static data due to internal error.

`MLResult MLSnapshotGetTransform(const  MLSnapshot  * snapshot, const  MLCoordinateFrameUID  * id, MLTransform  * out_transform)`
Get transform between world origin and the coordinate frame   id  .
- snapshot [in]: A snapshot of tracker state. Can be obtained with  MLPerceptionGetSnapshot() .
- id [in]: Look up the transform between the current origin and this coordinate frame id.
- out_transform [out]: Valid pointer to a  MLTransform . To be filled out with requested transform data.
Returns:
- `MLResult_InvalidParam` — Failed to obtain transform due to invalid parameter.
- `MLResult_Ok` — Obtained transform successfully.
- `MLResult_PoseNotFound` — Coordinate Frame is valid, but not found in the current pose snapshot.
- `MLResult_UnspecifiedFailure` — Failed to obtain transform due to internal error.

`MLResult MLSnapshotGetTransformWithDerivatives(const  MLSnapshot  * snapshot, const  MLCoordinateFrameUID  * id, MLTransform  * out_transform, MLTransformDerivatives  * out_derivatives)`
Get transform between world origin and the coordinate frame   id   as well as any derivatives that have been calculated.
API Level: 8
- snapshot [in]: A snapshot of tracker state. Can be obtained with  MLPerceptionGetSnapshot() .
- id [in]: Look up the transform between the current origin and this coordinate frame id.
- out_transform [out]: Valid pointer to a  MLTransform . To be filled out with requested transform data.
- out_derivatives [out]: Valid pointer to a  MLTransformDerivatives . To be filled out with the derivatives of the transform if available.
Returns:
- `MLResult_InvalidParam` — Failed to obtain transform due to invalid parameter.
- `MLResult_Ok` — Obtained transform successfully.
- `MLResult_PoseNotFound` — Coordinate Frame is valid, but not found in the current pose snapshot.
- `MLResult_UnspecifiedFailure` — Failed to obtain transform due to internal error.
- `MLSnapshotResult_DerivativesNotCalculated` — Derivatives are not available for the requested coordinate Frame.

`MLResult MLSnapshotGetPoseInBase(const  MLSnapshot  * snapshot, const  MLCoordinateFrameUID  * base_id, const  MLCoordinateFrameUID  * id, MLPose  * out_pose)`
Get transform between coordinate frame 'base_id' and the coordinate frame   id   as well as any derivatives that have been calculated.
API Level: 30
- snapshot [in]: A snapshot of tracker state. Can be obtained with  MLPerceptionGetSnapshot() .
- base_id [in]: The coordinate frame in which to locate 'id'.
- id [in]: The coordinate frame which needs to be located in the base_id coordinate frame.
- out_pose [out]: Valid pointer to a  MLPose . To be filled out with requested pose data.
Returns:
- `MLResult_InvalidParam` — Failed to obtain transform due to invalid parameter.
- `MLResult_Ok` — Obtained transform successfully.
- `MLResult_PoseNotFound` — Coordinate Frame is valid, but not found in the current pose snapshot.
- `MLResult_UnspecifiedFailure` — Failed to obtain transform due to internal error.

`const char * MLSnapshotGetResultString(MLResult result_code)`
Returns an ASCII string representation for each result code.
- result_code [in]: MLResult type to be converted to string.
