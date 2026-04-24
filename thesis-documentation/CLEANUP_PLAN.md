# Cleanup Plan for Handoff — `CppPlugin/depth_navigation/`

Preparing this codebase for handoff. Each step is self-contained and can be done independently. Build-verify after each step with `./gradlew assembleDebug`.

All paths relative to `CppPlugin/depth_navigation/app/src/main/cpp/`.

---

## Step 1: Remove Per-Frame Debug Logging

**File:** `main.cpp`  
**Lines:** 1119–1220  

Delete the entire block that dumps raw pixel values, center-region min/max, and confidence stats to logcat on every frame. This is ~100 lines of bring-up code. The actual work starts at line 1221 (`// ========== MARKER DETECTION INTEGRATION ==========`).

Also reduce `[DEBUG]` logging in `marker_detection.cpp` — multiple ALOGI calls fire per-blob per-frame (lines ~67, 75, 93–96, 110, 119–134, 224, 258–263, 285–288, 314–321, 338–348). Either delete or downgrade to `ALOGV`.

---

## Step 2: Resolve TODO Comments

**File:** `marker_detection.h`

| Line | Current TODO | Action |
|------|-------------|--------|
| 21 | "Decide on units - keep meters or convert to mm?" | Replace with: "Positions are in metres (ML2 native). Converted to mm at the tool_tracking boundary (tool_tracking.cpp:44-48)." |
| 22 | "Verify sphere_radius correction" | Confirm current behavior is correct and document it, or remove if the correction isn't applied |
| 23 | "Determine intensity threshold values" | Replace with: "Defaults tuned for retroreflective IR markers. Adjustable at runtime via GUI sliders." |
| 32 | "Calibrate with actual data" | Replace with: "Working defaults for current marker set" |
| 34 | "Verify physical marker size" | Confirm 5mm is correct for your markers, update comment |
| 38 | "does this actually make sense?" | Validate the area filter logic or remove it if it's not contributing |

**File:** `marker_detection.cpp` — resolve any remaining TODOs (line ~355).

---

## Step 3: Fix Naming Inconsistencies

### 3a. Hungarian notation in `tool_kalman_filter.h`
Rename member variables from Hungarian (`m_fMeasurementNoise`, `m_bInitialized`) to `snake_case_` (e.g., `measurement_noise_`, `initialized_`). ~10 variables, lines 95–99.

### 3b. Cryptic names in `tool_tracking.cpp`
- `tnid` → `tool_node_id`
- `cand` → `candidate`
- `sq_raw` → `squared_distance_raw`
- `id1`, `id2` → `marker_idx_a`, `marker_idx_b` (or similar)

### 3c. Global `using namespace` in `main.cpp:49`
`using namespace ml::app_framework;` at file scope. Move it inside the anonymous namespace (line 52) or into the class body.

---

## Step 4: Break Up `processFrame()`

**File:** `main.cpp`, starting at line 1118

After Step 1 removes debug logging, extract the remaining logic into helpers:

1. **`buildDetectionConfig()`** — lines ~1237–1249, builds `MarkerDetectionConfig` from GUI tuning variables
2. **`runMarkerDetection(frame)`** — lines ~1250–1290, calls `DetectMarkers()` and updates overlay nodes
3. **`updateToolCapture(markers)`** — lines ~1291–1320, the capture state machine
4. **`runToolTracking(markers)`** — lines ~1321–1344, calls `ToolTracker::ProcessFrame()` and outputs pose

Keep `processFrame()` as a short orchestrator that calls these four.

---

## Step 5: Break Up `UpdateGui()`

**File:** `main.cpp`, lines 263–532

Extract into panel-drawing helpers:
- `drawStreamInfoPanel()` — camera position, frame stats
- `drawStreamSettingsPanel()` — stream flags, depth range
- `drawTuningPanel()` — intensity thresholds, blur, morphology sliders
- `drawToolCapturePanel()` — capture button, tool list, status

Each panel is a self-contained ImGui section.

---

## Step 6: Break Up `TrackTool()`

**File:** `tool_tracking.cpp`, lines ~81–258

Extract:
- **`buildCandidateMatches()`** — the inner nested loop that finds distance-compatible marker pairs
- **`resolveConflicts()`** — the conflict resolution logic that picks the best match when tools share markers

---

## Step 7: Deduplicate Overlay Functions

**File:** `main.cpp`

`UpdateMarkerOverlay()` (lines ~721–760) and `UpdateRejectedOverlay()` (lines ~762–802) share ~80% of their code. Extract a shared helper:

```cpp
void updateOverlayNodes(const std::vector<DetectedMarker>& markers,
                        std::vector<Node>& nodes,
                        const glm::vec3& color);
```

Similarly, `UpdateMinDepth()` / `UpdateMaxDepth()` (lines ~557–575) are near-identical — unify into `UpdateDepthBound(bool is_min)`.

---

## Step 8: Document Complex Algorithms

**File:** `tool_tracking.cpp`

Add block comments explaining:

1. **Kabsch algorithm** (`MatchPointsKabsch`, lines ~320–455): What it does (finds optimal rotation+translation between two point sets), why it's used (aligns detected markers to tool template), and the mathematical approach (SVD of cross-covariance matrix).

2. **Shepperd's method** (lines ~402–409): Numerically stable rotation matrix → quaternion conversion. Note why this is preferred over naive extraction.

3. **Slerp** (lines ~428–431): Spherical linear interpolation for quaternion smoothing. Document the smoothing factor and its effect.

---

## Step 9: Replace Magic Numbers with Named Constants

Create constants at namespace scope (or in a config struct). Key candidates:

```cpp
// main.cpp
constexpr float kPreviewScaleFactor = 1.25f;          // line 79
constexpr float kPreviewDistanceM = -2.5f;             // line 86
constexpr glm::vec3 kGuiOffsetFromHead{0.25f, 0.f, -2.f}; // line 209
constexpr float kMarkerOverlayScale = 0.2f;            // line 812

// tool_tracking.cpp
constexpr float kMetresToMm = 1000.f;                  // line 46
constexpr float kSlerpIdentityThreshold = 0.9995f;     // line 424
constexpr float kQuatNormEpsilon = 1e-6f;              // line 435

// tool_kalman_filter.h
constexpr float kDefaultProcessNoise = 1e-4f;          // line 22
constexpr float kDefaultMeasurementNoise = 3.f;        // line 23
```

---

## Step 10: Fix Hidden Static State

**File:** `marker_detection.cpp`, lines ~204–206

Static locals `baseline_latched`, `z_initial`, `r_initial` persist across frames invisibly. Move them into a `DepthScalingState` struct:

```cpp
struct DepthScalingState {
    bool baseline_latched = false;
    float z_initial = 0.f;
    float r_initial = 0.f;
};
```

Have the caller (`DepthCameraApp`) own this struct and pass it into `DetectMarkers()`. This makes the state visible, testable, and resettable.

---

## Step 11: Build System Cleanup

1. **`app/build.gradle`**: Update `appcompat:1.0.2` → latest, `constraintlayout:1.1.3` → latest
2. **`app/build.gradle`**: Create `proguard-rules.pro` (referenced at line 45 but doesn't exist) or remove the reference
3. **`AndroidManifest.xml:8`**: Fix comment — says `hasCode="false"` but actual value is `true`
4. **`CMakeLists.txt:38`**: Add comment explaining why `DeprecatedApiUsage` is needed and what the migration path is

---

## Suggested Order

1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → 11

Steps 1–3 are quick wins that immediately improve readability. Steps 4–7 are structural refactors (do one function at a time, build-verify between). Steps 8–11 are polish.
