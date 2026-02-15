# Head Tracking

## Enums

### MLHeadTrackingError
A set of possible error conditions that can cause Head Tracking to be less than ideal.
API Level: 1

| Value | Description |
|---|---|
| `MLHeadTrackingError_None` | No error, tracking is nominal. |
| `MLHeadTrackingError_NotEnoughFeatures` | There are not enough features in the environment. |
| `MLHeadTrackingError_LowLight` | Lighting in the environment is not sufficient to track accurately. |
| `MLHeadTrackingError_Unknown` | Head tracking failed for an unknown reason. |

### MLHeadTrackingErrorFlag
A set of possible error conditions that can cause Head Tracking to be less than ideal.
API Level: 26

| Value | Description |
|---|---|
| `MLHeadTrackingErrorFlag_None = 0` | No error, tracking is nominal. |
| `MLHeadTrackingErrorFlag_Unknown = 1 << 0` | Head tracking failed for an unknown reason. |
| `MLHeadTrackingErrorFlag_NotEnoughFeatures = 1 << 1` | There are not enough features in the environment. |
| `MLHeadTrackingErrorFlag_LowLight = 1 << 2` | Lighting in the environment is not sufficient to track accurately. |
| `MLHeadTrackingErrorFlag_ExcessiveMotion = 1 << 3` | Head tracking failed due to excessive motion. |

### MLHeadTrackingMode
A set of possible tracking modes the Head Tracking system can be in.
API Level: 1

| Value | Description |
|---|---|
| `MLHeadTrackingMode_6DOF = 0` | Full 6 degrees of freedom tracking (position and orientation). |
| `MLHeadTrackingMode_Unavailable = 1` | Head tracking is unavailable. |

### MLHeadTrackingStatus
A set of possible tracking status for the Head Tracking system.
API Level: 26

| Value | Description |
|---|---|
| `MLHeadTrackingStatus_Invalid = 0` | Head tracking is unavailable. |
| `MLHeadTrackingStatus_Initializing = 1` | Head tracking is initializing. |
| `MLHeadTrackingStatus_Relocalizing = 2` | Head tracking is relocalizing. |
| `MLHeadTrackingStatus_Valid = 100` | Valid head tracking data is available. |

### MLHeadTrackingMapEvent
Different types of map events that can occur that a developer may have to handle.
API Level: 2

| Value | Description |
|---|---|
| `MLHeadTrackingMapEvent_Lost = (1 << 0)` | Map was lost. It could possibly recover. |
| `MLHeadTrackingMapEvent_Recovered = (1 << 1)` | Previous map was recovered. |
| `MLHeadTrackingMapEvent_RecoveryFailed = (1 << 2)` | Failed to recover previous map. |
| `MLHeadTrackingMapEvent_NewSession = (1 << 3)` | New map session created. |

## Functions

`typedef __attribute__()`
A structure containing information on the current state of the Head Tracking system.
API Level: 1
Returns:
- `MLResult_Ok` — Successfully created head tracker.
- `MLResult_UnspecifiedFailure` — Failed to create head tracker due to an unknown error.
- `MLResult_PermissionDenied` — The application lacks permission.
- `MLResult_Ok` — Successfully destroyed head tracker.
- `MLResult_UnspecifiedFailure` — Failed to destroy head tracker due to an unknown error.
- `MLResult_InvalidParam` — Failed to receive static data due to an invalid input parameter.
- `MLResult_Ok` — Successfully received static data.
- `MLResult_UnspecifiedFailure` — Failed to receive static data due to an unknown error.
- `MLResult_InvalidParam` — Failed to return the most recent head tracking state due to an invalid input parameter.
- `MLResult_Ok` — Successfully returned the most recent head tracking state.
- `MLResult_UnspecifiedFailure` — Failed to return the most recent head tracking state due to an unknown error.
- `MLResult_InvalidParam` — Failed to get map events due to an invalid input parameter.
- `MLResult_Ok` — Successfully got map events.
- `MLResult_UnspecifiedFailure` — Failed to get map events due to an unknown error.

` __attribute__()`
Returns the most recent head tracking state.
Returns:
- `MLResult_InvalidParam` — Failed to return the most recent head tracking state due to an invalid input parameter.
- `MLResult_Ok` — Successfully returned the most recent head tracking state.
- `MLResult_UnspecifiedFailure` — Failed to return the most recent head tracking state due to an unknown error.
