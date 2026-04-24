# Depth Camera

## Enums

### (constants)

| Value | Description |
|---|---|
| `MLDepthCameraIntrinsics_MaxDistortionCoefficients = 5` | Default distortion vector size. |

### MLDepthCameraStream
Enumeration of depth camera streams.
API Level: 29

| Value | Description |
|---|---|
| `MLDepthCameraStream_None = 0` | None. |
| `MLDepthCameraStream_LongRange = 1 << 0` | Long range stream. Under normal operations long range stream has a maximum frequency of 5fps and a range from 1m up to 5m, in some cases this can go as far 7.5m. |
| `MLDepthCameraStream_ShortRange = 1 << 1` | Short range stream. Under normal operations short range stream has a maximum frequency of 60fps and a range from 0.2m up to 0.9m. |

### MLDepthCameraFrameType
Enumeration of camera stream used when capturing a frame.
API Level: 29

| Value | Description |
|---|---|
| `MLDepthCameraFrameType_LongRange = 0` | Frame captured using  MLDepthCameraStream_LongRange  stream. |
| `MLDepthCameraFrameType_ShortRange = 1` | Frame captured using  MLDepthCameraStream_ShortRange  stream. |
| `MLDepthCameraFrameType_Count = 2` |  |

### MLDepthCameraFrameRate
Enumeration of possible frame rates.
API Level: 29

| Value | Description |
|---|---|
| `MLDepthCameraFrameRate_1FPS = 0` |  |
| `MLDepthCameraFrameRate_5FPS = 1` |  |
| `MLDepthCameraFrameRate_25FPS = 2` |  |
| `MLDepthCameraFrameRate_30FPS = 3` |  |
| `MLDepthCameraFrameRate_50FPS = 4` |  |
| `MLDepthCameraFrameRate_60FPS = 5` |  |

### MLDepthCameraFlags
Enumeration of flags to select data requested from depth camera.

| Value | Description |
|---|---|
| `MLDepthCameraFlags_None = 0` | None. |
| `MLDepthCameraFlags_DepthImage = 1 << 0` | Enable MLDepthCameraDepthImage. See  MLDepthCameraDepthImage  for more details. |
| `MLDepthCameraFlags_Confidence = 1 << 1` | Enable MLDepthCameraConfidenceBuffer. See  MLDepthCameraConfidenceBuffer  for more details. |
| `MLDepthCameraFlags_DepthFlags = 1 << 2` | Enable MLDepthCameraDepthFlagsBuffer. See  MLDepthCameraDepthFlagsBuffer  for more details. |
| `MLDepthCameraFlags_AmbientRawDepthImage = 1 << 3` | Enable MLDepthCameraAmbientRawDepthImage. See  MLDepthCameraAmbientRawDepthImage  for more details. |
| `MLDepthCameraFlags_RawDepthImage = 1 << 4` | Enable MLDepthCameraRawDepthImage. See  MLDepthCameraRawDepthImage  for more details. |

### MLDepthCameraDepthFlags
Enumeration of flags to select data requested from depth camera.

| Value | Description |
|---|---|
| `MLDepthCameraDepthFlags_Valid = 0 << 0` | Valid pixel. Indicates that there is no additional flag data for this pixel. |
| `MLDepthCameraDepthFlags_Invalid = 1 << 0` | Invalid. This bit is set to one to indicate that one or more flags from below have been set. Depending on the use case the application can correlate the flag data and corresponding pixel data to determine how to handle the pixel data. |
| `MLDepthCameraDepthFlags_Saturated = 1 << 1` | Pixel saturated. The pixel intensity is either below the min or the max threshold value. |
| `MLDepthCameraDepthFlags_Inconsistent = 1 << 2` | Inconsistent data. Inconsistent data received when capturing frames. This can happen due to fast motion. |
| `MLDepthCameraDepthFlags_LowSignal = 1 << 3` | Low signal. Pixel has very low signal to noise ratio. One example of when this can happen is for pixels in far end of the range. |
| `MLDepthCameraDepthFlags_FlyingPixel = 1 << 4` | Flying pixel. This typically happens when there is step jump in the distance of adjoining pixels in the scene. Example: When you open a door looking into the room the edges along the door's edges can cause flying pixels. |
| `MLDepthCameraDepthFlags_Masked = 1 << 5` | Masked. If this bit is on it indicates that the corresponding pixel may not be within the illuminator's illumination cone. |
| `MLDepthCameraDepthFlags_SBI = 1 << 8` | SBI. This bit will be set when there is high noise. |
| `MLDepthCameraDepthFlags_StrayLight = 1 << 9` | Stray light. This could happen when there is another light source apart from the depth camera illuminator. This could also lead to MLDepthCameraDepthFlags_LowSignal. |
| `MLDepthCameraDepthFlags_ConnectedComponent = 1 << 10` | Connected component. If a small group of MLDepthCameraDepthFlags_Valid is surrounded by a set of MLDepthCameraDepthFlags_Invalid then this bit will be set to 1. |

## Structs

### MLDepthCameraStreamConfig
Structure to encapsulate the camera config for a specific stream.
API Level: 29

- `uint32_t flags` — Flags to configure the depth data.
- `uint32_t exposure` — Exposure in microseconds.
- `MLDepthCameraFrameRate frame_rate` — Frame rate.

### MLDepthCameraSettings
Structure to encapsulate the camera settings.

- `uint32_t version` — Version of this structure.
- `uint32_t streams` — Depth camera stream.
- `MLDepthCameraStreamConfig stream_configs[MLDepthCameraFrameType_Count]` — Controls for each of the depth camera streams.

### MLDepthCameraIntrinsics
Depth camera intrinsic parameters.
API Level: 29

- `uint32_t width` — Camera width.
- `uint32_t height` — Camera height.
- `MLVec2f focal_length` — Camera focal length.
- `MLVec2f principal_point` — Camera principal point.
- `float fov` — Field of view in degrees.
- `double distortion[MLDepthCameraIntrinsics_MaxDistortionCoefficients]` — Distortion vector. The distortion co-efficients are in the following order: [k1, k2, p1, p2, k3].

### MLDepthCameraFrameBuffer
Structure to encapsulate per plane info for each camera frame.
API Level: 29

- `uint32_t width` — Width of the buffer in pixels.
- `uint32_t height` — Height of the buffer in pixels.
- `uint32_t stride` — Stride of the buffer in bytes.
- `uint32_t bytes_per_unit` — Number of bytes used to represent a single value.
- `uint32_t size` — Number of bytes in the buffer.
- `void * data` — Buffer data.

### MLDepthCameraFrame
Structure to encapsulate output data for each camera sensor.
API Level: 29

- `int64_t frame_number` — A 64bit integer to index the frame number associated with this frame.
- `MLTime frame_timestamp` — Frame timestamp specifies the time at which the frame was captured.
- `MLDepthCameraFrameType frame_type` — Depth camera stream used for capturing this frame.
- `MLTransform camera_pose` — Depth camera pose in the world co-ordinate system.
- `MLDepthCameraIntrinsics intrinsics` — Camera intrinsic parameters.
- `MLDepthCameraDepthImage  * depth_image` — Depth image. Depth is in **meters** (radial distance from the depth camera coordinate frame). Cast `data` to `float *`.
- `MLDepthCameraConfidenceBuffer  * confidence` — Confidence score.
- `MLDepthCameraDepthFlagsBuffer  * flags` — Depth flags.
- `MLDepthCameraAmbientRawDepthImage  * ambient_raw_depth_image` — Ambient raw depth image. The illuminator in the sensor is modulated with a system determined frequency. This is the raw sensor data captured when the illuminator is off.
- `MLDepthCameraRawDepthImage  * raw_depth_image` — Raw depth image. The illuminator in the sensor is modulated with a system determined frequency. This is the raw sensor data captured when the illuminator is on.

### MLDepthCameraData
Structure to encapsulate output data for each camera stream.
API Level: 29

- `uint32_t version` — Version of this structure.
- `uint8_t frame_count` — Number of camera frames populated.
- `MLDepthCameraFrame  * frames` — Camera frame data. The number of frames is specified by frame_count.

### MLDepthCameraStreamCapability
Structure to encapsulate a possible configuration for a single stream. Can be used to understand possible values for a specific #stream_configs element in  MLDepthCameraSettings .
API Level: 29

- `MLDepthCameraStream stream` — Stream for which this capability can be applied.
- `uint32_t min_exposure` — Minimum sensor exposure in microseconds.
- `uint32_t max_exposure` — Maximum sensor exposure in microseconds.
- `MLDepthCameraFrameRate frame_rate` — Frame rate.

### MLDepthCameraCapability
Structure to encapsulate a possible set of streams configuration. Such set describes a possible way of setting #stream_configs in  MLDepthCameraSettings  and may hold multiple  MLDepthCameraStreamCapability  for different streams.
API Level: 29

- `uint8_t size` — Size of  stream_capabilities  array.
- `MLDepthCameraStreamCapability  * stream_capabilities` — Array of  MLDepthCameraStreamCapability  elements.

### MLDepthCameraCapabilityList
Structure to encapsulate a list of possible stream configurations.
API Level: 29

- `uint8_t size` — Size of  capabilities  array.
- `MLDepthCameraCapability  * capabilities` — Array of  MLDepthCameraCapability  elements.

### MLDepthCameraCapabilityFilter
Structure to encapsulate camera capabilities filtering.
API Level: 29

- `uint32_t version` — Version of this structure.
- `uint32_t streams` — Streams for which capabilities will be filtered.

## Functions

`void MLDepthCameraSettingsInit(MLDepthCameraSettings  * inout_handle)`
Initialize the connect handle structure. Shall be called before calling  MLDepthCameraConnect() .
API Level: 29
- inout_handle [inout]: MLDepthCameraSettings  structure to initialize.

`void MLDepthCameraDataInit(MLDepthCameraData  * inout_depth_camera_data)`
Initialize  MLDepthCameraData  with version.
API Level: 29
- inout_depth_camera_data [inout]: Set up the version for inout_depth_camera_data.

`void MLDepthCameraCapabilityFilterInit(MLDepthCameraCapabilityFilter  * inout_handle)`
Initialize the capabilities filtering structure. Shall be called before calling  MLDepthCameraGetCapabilities() .
API Level: 29
- inout_handle [inout]: MLDepthCameraCapabilityFilter  structure to initialize.

`MLResult MLDepthCameraConnect(const  MLDepthCameraSettings  * settings, MLHandle  * out_handle)`
Connect to depth camera.
API Level: 29
Permission: `com.magicleap.permission.DEPTH_CAMERA (protection level: dangerous)`
- settings [in]: A pointer to  MLDepthCameraSettings  structure.
- out_handle [out]: A pointer to camera handle to be used in later APIs.
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_Ok` — Connected to camera device(s) successfully.
- `MLResult_PermissionDenied` — Necessary permission is missing.
- `MLResult_UnspecifiedFailure` — The operation failed with an unspecified error.

`MLResult MLDepthCameraUpdateSettings(MLHandle handle, const  MLDepthCameraSettings  * settings)`
Update the depth camera settings.
API Level: 29
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
- settings [in]: Pointer to  MLDepthCameraSettings .
Returns:
- `MLResult_InvalidParam` — Invalid handle.
- `MLResult_Ok` — Settings updated successfully.
- `MLResult_UnspecifiedFailure` — Failed due to internal error.

`MLResult MLDepthCameraGetCapabilities(MLHandle handle, const  MLDepthCameraCapabilityFilter  * filter, MLDepthCameraCapabilityList  * out_caps)`
Query the depth camera stream capabilities.
API Level: 29
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
- filter [in]: Pointer to initialized  MLDepthCameraCapabilityFilter  structure. Used for filtering.
- out_caps [out]: Pointer to initialized  MLDepthCameraCapabilityList  structure.
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_Ok` — API call completed successfully.

`MLResult MLDepthCameraReleaseCapabilities(MLHandle handle, MLDepthCameraCapabilityList  * out_caps)`
Release resources allocated with  MLDepthCameraGetCapabilities .
API Level: 29
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
- out_caps [out]: Pointer to  MLDepthCameraCapabilityList  filled by the call to  MLDepthCameraGetCapabilities .
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_Ok` — API call completed successfully.

`MLResult MLDepthCameraGetLatestDepthData(MLHandle handle, uint64_t timeout_ms, MLDepthCameraData  * out_data)`
Poll for Frames.
API Level: 29
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
- timeout_ms [in]: Timeout in milliseconds.
- out_data [out]: Depth camera data. Should be an initialized  MLDepthCameraData  object.
Returns:
- `MLResult_InvalidParam` — Invalid handle.
- `MLResult_Ok` — Depth camera data fetched successfully.
- `MLResult_Timeout` — Returned because no new frame available at this time.
- `MLResult_UnspecifiedFailure` — Failed due to internal error.

`MLResult MLDepthCameraReleaseDepthData(MLHandle handle, MLDepthCameraData  * depth_camera_data)`
Releases specified  MLDepthCameraData  object.
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
- depth_camera_data [in]: Pointer to a valid  MLDepthCameraData  object.
Returns:
- `MLResult_Ok` — Successfully released depth camera data.
- `MLResult_InvalidParam` — depth_camera_data parameter was not valid (NULL).
- `MLResult_UnspecifiedFailure` — Failed due to internal error.

`MLResult MLDepthCameraDisconnect(MLHandle handle)`
Disconnect from depth camera.
API Level: 29
- handle [in]: Camera handle obtained from  MLDepthCameraConnect .
Returns:
- `MLResult_InvalidParam` — Invalid handle.
- `MLResult_Ok` — Disconnected camera successfully.
- `MLResult_UnspecifiedFailure` — Failed to disconnect camera.
