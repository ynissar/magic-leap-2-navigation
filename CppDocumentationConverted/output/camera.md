# Camera

## Enums

### (constants)

| Value | Description |
|---|---|
| `MLCAMERA_MAXSTREAMS = 2` | Max No of streams supported by logical camera. |

### MLCameraIdentifier
Logical camera identifiers available for access.

| Value | Description |
|---|---|
| `MLCameraIdentifier_MAIN = 0` | x86 logical camera. |
| `MLCameraIdentifier_CV` | CV logical camera. |

### MLCameraCaptureType
Captures operation type.

| Value | Description |
|---|---|
| `MLCameraCaptureType_Image = 0` | To capture an image. |
| `MLCameraCaptureType_Video` | To capture a video. |
| `MLCameraCaptureType_Preview` | To capture a video and access the raw buffer of the frames. |

### MLCameraCaptureFrameRate
Captures frame rate.

| Value | Description |
|---|---|
| `MLCameraCaptureFrameRate_None = 0` | None, used for still capture. |
| `MLCameraCaptureFrameRate_15FPS` | Specified 15FPS. |
| `MLCameraCaptureFrameRate_30FPS` | Specified 30FPS. |
| `MLCameraCaptureFrameRate_60FPS` | Specified 60FPS. |

### MLCameraMRQuality
Video Quality enumeration for mixed reality capture.

| Value | Description |
|---|---|
| `MLCameraMRQuality_648x720 = 1` | Specifies 648 x 720 resolution. Aspect ratio: 9x10. |
| `MLCameraMRQuality_972x1080 = 2` | Specifies 972 x 1080 resolution. Aspect ratio: 9x10. |
| `MLCameraMRQuality_1944x2160 = 3` | Specifies 1944 x 2160 resolution. Aspect ratio: 9x10. MLCameraCaptureFrameRate_60FPS is not supported for this quality mode. |
| `MLCameraMRQuality_960x720 = 4` | Specifies 960 x 720 resolution. Aspect ratio: 4x3. |
| `MLCameraMRQuality_1440x1080 = 5` | Specifies 1440 x 1080 resolution. Aspect ratio: 4x3. |
| `MLCameraMRQuality_2880x2160 = 6` | Specifies 2880 x 2160 resolution. Aspect ratio: 4x3. MLCameraCaptureFrameRate_60FPS is not supported for this quality mode. |

### MLCameraConnectFlag
Flags to describe various modules in camera pipeline.

| Value | Description |
|---|---|
| `MLCameraConnectFlag_CamOnly = 0x0` | Camera only frame capture. |
| `MLCameraConnectFlag_VirtualOnly = 0x1` | Virtual only capture. Only supported for MLCameraIdentifier_MAIN. |
| `MLCameraConnectFlag_MR = 0x2` | Mixed Reality capture. Only supported for MLCameraIdentifier_MAIN. |

### MLCameraMRBlendType
Virtual and real content blending modes.

| Value | Description |
|---|---|
| `MLCameraMRBlendType_Additive = 1` | Additive blend type. It simply adds pixel values of real world and virtual layer. |

### MLCameraDisconnectReason
Camera disconnect reason.

| Value | Description |
|---|---|
| `MLCameraDisconnect_DeviceLost = 0` | Device lost. |
| `MLCameraDisconnect_PriorityLost` | Priority lost. |

### MLCameraError
Camera errors.

| Value | Description |
|---|---|
| `MLCameraError_None = 0` |  |
| `MLCameraError_Invalid` | Invalid/Unknown Error. |
| `MLCameraError_Disabled` | Camera disabled. |
| `MLCameraError_DeviceFailed` | Camera device failed. |
| `MLCameraError_ServiceFailed` | Camera service failed. |
| `MLCameraError_CaptureFailed` | Capture failed. |

### MLCameraOutputFormat
Captured output format. These three parameters determine which formats are supported:

| Value | Description |
|---|---|
| `MLCameraOutputFormat_Unknown` |  |
| `MLCameraOutputFormat_YUV_420_888` | YUV planar format. |
| `MLCameraOutputFormat_JPEG` | Compressed output stream. |
| `MLCameraOutputFormat_RGBA_8888` | RGB32 format. |

### MLCameraDeviceStatusFlag
Client can implement polling mechanism to retrieve device status and use these masks to view device status.

| Value | Description |
|---|---|
| `MLCameraDeviceStatusFlag_Connected = 1 << 0` | Connected. |
| `MLCameraDeviceStatusFlag_Idle = 1 << 1` | Idle. |
| `MLCameraDeviceStatusFlag_Streaming = 1 << 2` | Opened. |
| `MLCameraDeviceStatusFlag_Disconnected = 1 << 3` | Disconnected. |
| `MLCameraDeviceStatusFlag_Error = 1 << 4` | Error. Call  MLCameraGetErrorCode  to obtain the error code. |

### (constants)

| Value | Description |
|---|---|
| `MLCamera_MaxImagePlanes = 3` | Number of planes representing the image color space. |

### (constants)
Camera distortion vector size.
API Level: 20

| Value | Description |
|---|---|
| `MLCameraIntrinsics_MaxDistortionCoefficients = 5` | Default distortion vector size. |

## Structs

### MLCameraMRConnectInfo
A structure to encapsulate connection settings for MR capture.

- `MLCameraMRQuality quality` — Video quality.
- `MLCameraMRBlendType blend_type` — Blending type for mixed reality capture.
- `MLCameraCaptureFrameRate frame_rate` — Capture frame rate.

### MLCameraConnectContext
A structure to encapsulate context for a CameraConnect Request.

- `uint32_t version` — Version contains the version number for this structure.
- `MLCameraIdentifier cam_id` — Logical camera identifier.
- `MLCameraConnectFlag flags` — Context in which the camera should operate in.
- `bool enable_video_stab` — Enable/disable video stabilization.
- `MLCameraMRConnectInfo mr_info` — MR capture connection settings.

### MLCameraCaptureStreamCaps
A structure to encapsulate stream capabilities.
API Level: 20

- `MLCameraCaptureType capture_type` — Capture type: video, image, or preview.
- `int32_t width` — Resolution width.
- `int32_t height` — Resolution height.

### MLCameraCaptureStreamConfig
A structure to encapsulate stream configurations.

- `MLCameraCaptureType capture_type` — Capture type.
- `int32_t width` — Specifies resolution width.
- `int32_t height` — Specifies resolution height.
- `MLCameraOutputFormat output_format` — Specifies output format.
- `MLHandle native_surface_handle` — Native surface.

### MLCameraCaptureConfig
A structure to encapsulate capture configuration.

- `uint32_t version` — Version of this structure.
- `MLCameraCaptureFrameRate capture_frame_rate` — Capture frame rate:   
If only IMAGE stream configuration set to MLCameraCaptureFrameRate_None.   
If setting to 60fps capture resolution should not be more than 1920x1080 for any of the streams.   
For MR/virtual only capture, the frame rate should match the value selected in  MLCameraMRConnectInfo .
- `uint32_t num_streams` — Number of captured streams.
- `MLCameraCaptureStreamConfig stream_config[MLCAMERA_MAXSTREAMS]` — Stream configurations.

### MLCameraPlaneInfo
Per plane info for captured output.

- `uint32_t version` — Version of this structure.
- `uint32_t width` — Width of the output image in pixels.
- `uint32_t height` — Height of the output image in pixels.
- `uint32_t stride` — Stride of the output image in bytes.
- `uint32_t bytes_per_pixel` — Number of bytes used to represent a pixel.
- `uint32_t pixel_stride` — Distance between 2 consecutive pixels in bytes.
- `uint8_t * data` — Image data.
- `uint32_t size` — Number of bytes in the image output data.

### MLCameraOutput
A structure to encapsulate captured output.

- `uint32_t version` — Version of this structure.
- `uint8_t plane_count` — Number of output image planes:   
1 for compressed output such as JPEG stream,   
3 for separate color component output such as YUV/RGB.
- `MLCameraPlaneInfo planes[MLCamera_MaxImagePlanes]` — Output image plane info. The number of output planes is specified by plane_count.
- `MLCameraOutputFormat format` — Supported output format specified by MLCameraOutputFormat.

### MLCameraIntrinsicCalibrationParameters
Camera intrinsic parameter.
API Level: 20

- `uint32_t version` — Version of this structure.
- `uint32_t width` — Camera width.
- `uint32_t height` — Camera height.
- `MLVec2f focal_length` — Camera focal length.
- `MLVec2f principal_point` — Camera principal point.
- `float fov` — Field of view in degrees.
- `double distortion[MLCameraIntrinsics_MaxDistortionCoefficients]` — Distortion vector. The distortion co-efficients are in the following order:   
[k1, k2, p1, p2, k3].

### MLCameraResultExtras
A structure to encapsulate various indices for a capture result.

- `uint32_t version` — Version of this structure.
- `int64_t frame_number` — A 64bit integer to index the frame number associated with this result.
- `MLTime vcam_timestamp` — Frame timestamp.
- `MLCameraIntrinsicCalibrationParameters  * intrinsics` — Camera intrinsic parameter. 
 Only valid within callback scope. 
 The Library allocates and maintains the lifetime of intrinsics. 
 Only valid for on_image_buffer_available, on_video_buffer_available, on_preview_buffer_available callbacks. 
 NULL for on_capture_completed, on_capture_failed callbacks.

### MLCameraDeviceAvailabilityInfo
A structure to represent info on camera availability.

- `MLCameraIdentifier cam_id` — Identifier for the camera that the callback applies.
- `void * user_data` — The context pointer supplied to  MLCameraInit()  call.

### MLCameraDeviceAvailabilityStatusCallbacks
Device availability status callbacks to be implemented by client to receive device availability status.

- `uint32_t version` — Version of this structure.
- `void(* on_device_available)(const MLCameraDeviceAvailabilityInfo *info)` — Callback is invoked when the camera becomes available.
- `void(* on_device_unavailable)(const MLCameraDeviceAvailabilityInfo *info)` — Callback is invoked when the camera becomes unavailable.

### MLCameraDeviceStatusCallbacks
Device status callbacks to be implemented by client to receive device status if callback mechanism is used.

- `uint32_t version` — Version of this structure.
- `void(* on_device_streaming)(void *data)` — Callback is invoked when the camera is streaming.
- `void(* on_device_idle)(void *data)` — Callback is invoked when the camera stops streaming.
- `void(* on_device_disconnected)(MLCameraDisconnectReason reason, void *data)` — Callback is invoked when the camera is disconnected.
- `void(* on_device_error)(MLCameraError error, void *data)` — Callback is invoked when the camera encountered errors.

### MLCameraCaptureCallbacks
Capture callbacks to be implemented by client to receive capture status if callback mechanism is used.

- `uint32_t version` — Version of this structure.
- `void(* on_capture_failed)(const MLCameraResultExtras *extra, void *data)` — Callback is invoked when a capture has failed when the camera device failed to produce a capture result for the request.
- `void(* on_capture_aborted)(void *data)` — Callback is invoked when an ongoing video or preview capture or both are aborted due to an error. This is not valid for MR Capture.
- `void(* on_capture_completed)(MLHandle result_metadata_handle, const MLCameraResultExtras *extra, void *data)` — Callback is invoked when capturing single frame is completed and result is available. For MRCapture null metadata handle.
- `void(* on_image_buffer_available)(const MLCameraOutput *output, const MLHandle result_metadata_handle, const MLCameraResultExtras *extra, void *data)` — Callback is invoked when a captured image buffer is available with  MLCameraCaptureType_Image .
- `void(* on_video_buffer_available)(const MLCameraOutput *output, const MLHandle result_metadata_handle, const MLCameraResultExtras *extra, void *data)` — Callback is invoked when a captured video frame buffer is available with  MLCameraCaptureType_Video .
- `void(* on_preview_buffer_available)(const MLHandle buffer_handle, const MLHandle result_metadata_handle, const MLCameraResultExtras *extra, void *data)` — Callback is invoked when a preview video frame buffer is available with  MLCameraCaptureType_Preview .

## Functions

`void MLCameraConnectContextInit(MLCameraConnectContext  * inout_context)`
Initialize the connect context structure.
API Level: 20
- inout_context [inout]: MLCameraConnectContext  structure to initialize.

`void MLCameraCaptureConfigInit(MLCameraCaptureConfig  * inout_config)`
Initialize the capture config structure.
API Level: 20
- inout_config [inout]: MLCameraCaptureConfig  structure to initialize.

`void MLCameraDeviceAvailabilityStatusCallbacksInit(MLCameraDeviceAvailabilityStatusCallbacks  * inout_device_availability_status_callbacks)`
Initialize the callback structure.
API Level: 20
- inout_device_availability_status_callbacks [inout]: Device availability status callbacks structure to initialize.

`void MLCameraDeviceStatusCallbacksInit(MLCameraDeviceStatusCallbacks  * inout_device_status_callbacks)`
Initialize the callback structure.
API Level: 20
- inout_device_status_callbacks [inout]: Device status callbacks structure to initialize.

`void MLCameraCaptureCallbacksInit(MLCameraCaptureCallbacks  * inout_capture_callbacks)`
Initialize the callback structure.
API Level: 20
- inout_capture_callbacks [inout]: Capture status callbacks structure to initialize.

`MLResult MLCameraInit(const  MLCameraDeviceAvailabilityStatusCallbacks  * device_availability_status_callbacks, void * user_data)`
Initialize ML Camera API, Register callback for device availability. If the user does not want to register device availability listener, MLCameraConnect can be called directly without  MLCameraInit() . MLCameraDeInit should be used for unregistering callbacks and releasing resources acquired in  MLCameraInit() .
API Level: 20
Permission: `android.permission.CAMERA (protection level: dangerous)`
- device_availability_status_callbacks [in]: Callback to notify camera availability status.
- user_data [in]: Pointer to user context data (can be NULL).
Returns:
- `MLResult_Ok` — Camera is initialized and callbacks registered successfully.
- `MLResult_UnspecifiedFailure` — The operation failed with an unspecified error.
- `MLResult_PermissionDenied` — Necessary permission is missing.

`MLResult MLCameraConnect(const  MLCameraConnectContext  * input_context, MLCameraContext  * out_context)`
Connect to camera device.
API Level: 20
Permission: `android.permission.CAMERA (protection level: dangerous)`
- input_context [in]: MLCameraConnectContext  structure.
- out_context [out]: Camera context to be used in later APIs.
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_UnspecifiedFailure` — The operation failed with an unspecified error.
- `MLResult_Ok` — Connected to camera device successfully.
- `MLMediaGenericResult_InvalidOperation` — camera device already connected.
- `MLResult_PermissionDenied` — Necessary permission is missing.

`MLResult MLCameraGetNumSupportedStreams(MLCameraContext context, uint32_t * out_num_supported_streams)`
Query the no of streams supported by camera device.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- out_num_supported_streams [out]: Number of streams supported by device.
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_Ok` — API call completed successfully.

`MLResult MLCameraGetStreamCaps(MLCameraContext context, const uint32_t stream_index, uint32_t * inout_num_stream_caps, MLCameraCaptureStreamCaps  * inout_stream_caps)`
Query the stream capabilities.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- stream_index [in]: Index of the stream for which to query capabilities. The index should be in the range [0 , num_supported_streams) where the num_supported_streams is from  MLCameraGetNumSupportedStreams() .
- inout_num_stream_caps [inout]: When the parameter inout_stream_caps is null, this is an output parameter which will hold the number of capabilities supported by the stream on function return. Otherwise this is an input parameter specifying the number of capabilities to retrieve, namely the size of the array pointed to by inout_stream_caps on return. The value should be in the range of [0, number-of-caps-retrieved].
- inout_stream_caps [inout]: This is either null or pointing to an array of  MLCameraCaptureStreamCaps()  on return. Note that caller is responsible for allocating and releasing the array.
Returns:
- `MLResult_InvalidParam` — One of the parameters is invalid.
- `MLResult_Ok` — API call completed successfully.

`MLResult MLCameraDisconnect(MLCameraContext context)`
Disconnect from camera device.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_Ok` — Disconnected Camera device successfully.
- `MLMediaGenericResult_InvalidOperation` — Camera device already disconnected or camera device is streaming.

`MLResult MLCameraDeInit()`
Uninitialize ML Camera API, unregister callback for device availability. Should be called after all camera devices are disconnected. After MLCameraDeInit, MLCameraInit can be called or MLCameraConnect can be called.
API Level: 20
Returns:
- `MLResult_Ok` — Deinitialization completed successfully.
- `MLMediaGenericResult_InvalidOperation` — DeInit called when camera device is connected.

`MLResult MLCameraPreCaptureAEAWB(MLCameraContext context)`
Trigger AEAWB Convergence.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid camera context.
- `MLResult_Ok` — AEAWB convergence done.
- `MLResult_UnspecifiedFailure` — Unspecified failure.
- `MLResult_Timeout` — Timed out waiting for AEAWB convergence.
- `MLMediaGenericResult_InvalidOperation` — Camera device not configured using  MLCameraPrepareCapture()  or camera device is streaming.

`MLResult MLCameraPrepareCapture(MLCameraContext context, const  MLCameraCaptureConfig  * config, MLHandle  * out_request_handle)`
Prepare for capture.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- config [in]: Capture configuration.
- out_request_handle [out]: Handle to the capture request metadata. Only valid if result is MLResult_Ok.
Returns:
- `MLResult_InvalidParam` — Failed to prepare for capture due to invalid input parameter.
- `MLResult_Ok` — Prepared for capture successfully.
- `MLResult_UnspecifiedFailure` — Failed to prepare for capture due to internal error.
- `MLMediaGenericResult_InvalidOperation` — Camera device in streaming state.

`MLResult MLCameraUpdateCaptureSettings(MLCameraContext context)`
Update capture setting.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLMediaGenericResult_InvalidOperation` — Camera not streaming video or not in preview.

`MLResult MLCameraSetDeviceStatusCallbacks(MLCameraContext context, const  MLCameraDeviceStatusCallbacks  * device_status_callbacks, void * data)`
Set the client-implemented callbacks to convey camera device status.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- device_status_callbacks [in]: Camera device status callbacks.
- data [in]: User metadata.
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLResult_Ok` — Set device status callbacks successfully.

`MLResult MLCameraSetCaptureCallbacks(MLCameraContext context, const  MLCameraCaptureCallbacks  * capture_callbacks, void * data)`
Set the client-implemented callbacks to convey capture status.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- capture_callbacks [in]: Capture status callbacks.
- data [in]: User metadata.
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLResult_Ok` — Set Capture callbacks successfully.
- `MLResult_UnspecifiedFailure` — Internal error occurred.

`MLResult MLCameraCaptureImage(MLCameraContext context, uint32_t num_images)`
Capture still image.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- num_images [in]: Number of images to capture valid range is 1-10.
Returns:
- `MLResult_InvalidParam` — Invalid context or invalid num_images.
- `MLResult_Ok` — Triggered image capture successfully.
- `MLMediaGenericResult_InvalidOperation` — MLMediaGenericResult_InvalidOperation Capture device in invalid state or image stream not configured by  MLCameraPrepareCapture() .
- `MLResult_UnspecifiedFailure` — Internal error occurred.

`MLResult MLCameraCaptureVideoStart(MLCameraContext context)`
Start video capture. Capture either encoded video or YUV/RGBA frames.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid context input parameter.
- `MLResult_Ok` — Started video capture successfully.
- `MLMediaGenericResult_InvalidOperation` — Capture device in invalid state or video stream not configured by  MLCameraPrepareCapture() .
- `MLResult_UnspecifiedFailure` — Failed to start video recording due to internal error.

`MLResult MLCameraCapturePreviewStart(MLCameraContext context)`
Start preview provide raw frames through callback.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLResult_Ok` — Started preview successfully.
- `MLResult_UnspecifiedFailure` — Failed to start preview due to internal error.
- `MLMediaGenericResult_InvalidOperation` — Capture device in invalid state or preview stream not configured by  MLCameraPrepareCapture() .

`MLResult MLCameraCaptureVideoStop(MLCameraContext context)`
Stop video capture.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLResult_Ok` — Stopped video capture successfully.
- `MLResult_UnspecifiedFailure` — Failed to stop video recording due to internal error.
- `MLMediaGenericResult_InvalidOperation` — Capture device in invalid state or video stream not streaming.

`MLResult MLCameraCapturePreviewStop(MLCameraContext context)`
Stop preview.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
Returns:
- `MLResult_InvalidParam` — Invalid context.
- `MLResult_Ok` — Stopped video capture successfully.
- `MLResult_UnspecifiedFailure` — Failed to stop preview due to internal error.
- `MLMediaGenericResult_InvalidOperation` — Capture device in invalid state or preview stream not streaming.

`MLResult MLCameraGetDeviceStatus(MLCameraContext context, uint32_t * out_device_status)`
Poll camera device status.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- out_device_status [out]: Device status.
Returns:
- `MLResult_InvalidParam` — Failed to obtain device status due to invalid input parameter.
- `MLResult_Ok` — Obtained device status successfully.

`MLResult MLCameraGetDeviceAvailabilityStatus(MLCameraIdentifier cam_id, bool * out_device_availability_status)`
Poll camera device availability status.
API Level: 20
- cam_id [in]: Camera Id for which the availability status is to be queried.
- out_device_availability_status [out]: Device availability status.
Returns:
- `MLResult_InvalidParam` — Failed to obtain device status due to invalid input parameter.
- `MLResult_Ok` — Obtained device status successfully.

`MLResult MLCameraGetErrorCode(MLCameraContext context, MLCameraError  * out_error_code)`
Obtain camera device error code.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- out_error_code [out]: Camera device error code.
Returns:
- `MLResult_InvalidParam` — Failed to obtain device error code due to invalid input parameter.
- `MLResult_Ok` — Obtained camera device error code successfully.

`MLResult MLCameraGetCameraCharacteristics(MLCameraContext context, MLHandle  * out_characteristics_handle)`
Obtains handle for retrieving camera characteristics.
API Level: 20
- context [in]: Camera context obtained from  MLCameraConnect() .
- out_characteristics_handle [out]: Handle to access camera characteristic metadata. Only valid if result is MLResult_Ok.
Returns:
- `MLResult_InvalidParam` — Failed to obtain camera characteristic handle due to invalid input parameter.
- `MLResult_Ok` — Obtained camera characteristic handle successfully.
- `MLResult_UnspecifiedFailure` — Failed due to internal error.
