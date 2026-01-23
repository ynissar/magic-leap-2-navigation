# HL2IRToolTracking.cpp Summary

This file implements **infrared (IR) surgical tool tracking** on the HoloLens 2 using the depth sensor (AHAT - Articulated Hand Tracking camera).

---

## 1. Initialization (Constructor: lines 35-76)

**What it does:** Loads the Research Mode API and sets up sensor access.

**How it works:**
- Loads the `ResearchModeAPI` DLL dynamically using `LoadLibraryA`
- Creates a sensor device using `CreateResearchModeSensorDevice`
- Gets the **spatial locator** from the "rig node" (the sensor's coordinate frame relative to the world)
- Requests camera and IMU access permissions asynchronously
- Enumerates all available sensors on the device

---

## 2. Consent Checking (lines 78-156)

**What it does:** Validates that the user has granted permission to access sensors.

**How it works:**
- `CheckCamConsent()` and `CheckImuConsent()` wait for consent events
- Returns appropriate error codes if access is denied (by system, user, or not declared in app manifest)
- This is required because Research Mode requires explicit user consent

---

## 3. Depth Sensor Initialization (lines 157-171)

**What it does:** Finds and configures the AHAT depth sensor.

**How it works:**
- Iterates through available sensors looking for `DEPTH_AHAT`
- Gets the sensor interface and queries camera extrinsics (the 4x4 transformation matrix from camera to rig)
- Pre-computes the **inverse extrinsics matrix** for later coordinate transformations

---

## 4. Depth Sensor Loop (lines 173-353)

This is the **main processing loop** - the heart of the system.

### Step 4a: Stream Opening (lines 188-189)
- Opens the depth sensor data stream

### Step 4b: Frame Acquisition (lines 199-207)
- Continuously calls `GetNextBuffer()` to get new depth frames
- Checks timestamp to skip duplicate frames

### Step 4c: Buffer Extraction (lines 214-223)
- Gets the **depth buffer** (`pDepth`) - distance values
- Gets the **active brightness (AB) buffer** (`pAbImage`) - IR reflectance image used for tracking

### Step 4d: Coordinate Transform Computation (lines 229-272)
- Converts the frame timestamp to a `PerceptionTimestamp`
- Uses the spatial locator to get the **camera-to-world transform** at that timestamp
- Computes `depthToWorld` matrix: `cameraExtrinsicsInverse × rigToWorld`
- Extracts rotation (3x3) and translation (3x1) components

### Step 4e: Look-Up Table (LUT) Generation (lines 274-313)
- **First frame only:** Builds a LUT mapping each pixel (u,v) to a normalized 3D ray direction
- Uses `MapImagePointToCameraUnitPlane()` to get the ray direction for each pixel
- Normalizes the directions so they can be multiplied by depth to get 3D points
- This is cached for performance

### Step 4f: Tool Tracking (lines 316-325)
- If the `IRToolTracker` is active and there's a new frame:
  - Passes the AB image, depth image, resolution, and camera-to-world transform to the tracker
  - The tracker processes this to find IR reflective spheres and compute tool poses

### Step 4g: Cleanup (lines 328-352)
- Releases frame buffers
- On loop exit, closes the stream and releases sensor resources

---

## 5. Tool Definition Management (lines 436-483)

**What it does:** Allows defining trackable tools by their sphere geometry.

**How it works:**
- `AddToolDefinition()` takes:
  - Number of spheres
  - Sphere positions (x, y, z for each)
  - Sphere radius
  - Tool identifier string
  - Minimum visible spheres for detection
  - Lowpass filter coefficients for smoothing
- Converts Unity coordinates (meters, left-handed) to tracking coordinates (millimeters, flipped Z)
- Creates an `IRToolTracker` if not already initialized
- Registers the tool geometry with the tracker

---

## 6. Tracking Control (lines 485-513)

**What it does:** Starts and stops the tracking pipeline.

### `StartToolTracking()`:
1. Initializes `IRToolTracker` if needed
2. Starts the depth sensor loop (on a separate thread)
3. Starts the tool tracking algorithm

### `StopToolTracking()`:
1. Stops the tracker
2. Sets `m_depthSensorLoopStarted = false` to exit the loop
3. Joins the depth thread to ensure clean shutdown

---

## 7. Tool Transform Retrieval (lines 516-533)

**What it does:** Returns the current pose of a tracked tool.

**How it works:**
- Queries the `IRToolTracker` for the tool's transform by identifier
- Returns the 4x4 transformation matrix as a flat float array
- Returns zeros if tracking is not active

---

## Key Data Flow

```
Depth Sensor → Frame (Depth + IR) → Coordinate Transform → IRToolTracker
                                                               ↓
                             Tool Pose (4x4 matrix) ← Sphere Detection & Registration
```

The system essentially uses the IR brightness image to detect reflective spheres, matches them against known tool geometries, and outputs the tool's 6DoF pose in world coordinates.
