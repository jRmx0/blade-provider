# algo_bounce Early Exit Analysis

## Overview
`bounce_run_pipeline()` can exit without hitting any configured target due to several safety caps and failure conditions. This document maps all early exit paths.

## Pipeline Main Loop
**Location:** [bounce_runner.c](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L628)

```c
while (!bounce_targets_reached(&ctx, environment) &&
       ((max_iterations == 0u)
            ? (ctx.metrics.iteration < BOUNCE_MAX_SEGMENTS_HARD_CAP)
            : (ctx.metrics.iteration < (int)max_iterations)))
```

The loop continues **only while:**
1. Targets are NOT reached, AND
2. Iteration count is below its cap

## Target Detection
**Function:** `bounce_targets_reached()` ([line 570](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L570))

Targets are only considered reached if **at least one** is met:
```c
if (env->target_distance > 0.001f && ctx->metrics.total_distance >= env->target_distance)
    return true;
if (env->target_coverage > 0.001f && ctx->metrics.estimated_coverage >= env->target_coverage)
    return true;
return false;
```

**Key:** If neither target_distance nor target_coverage is configured (or both are ≤ 0.001), targets are **never** reached, and only `max_iterations` matters.

## Early Exit Paths (No Target Hit)

### 1. **Max Iterations Exceeded** (Primary)
- **Condition:** `ctx.metrics.iteration >= max_iterations` (when configured)
- **Lines:** [628-629](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L628-L629)
- **Impact:** Loop exit condition naturally fails
- **Common:** When `max_iterations` is set but targets aren't reached by that point

### 2. **Total Attempts Safety Cap Exceeded**
- **Constant:** `BOUNCE_MAX_TOTAL_ATTEMPTS = 600,000`
- **Lines:** [631-635](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L631-L635)
- **Condition:** `total_attempts > 600,000` (counts all attempts, including retries)
- **Trigger:** Excessive retry loops from validation failures or float-lerp drift
- **Message:** `"aborting after %d total attempts (memory/loop safety cap)"`

```c
if (total_attempts > BOUNCE_MAX_TOTAL_ATTEMPTS) {
    printf("bounce_run_pipeline: aborting after %d total attempts...\n", total_attempts);
    break;
}
```

### 3. **Segments Hard Cap Exceeded**
- **Constant:** `BOUNCE_MAX_SEGMENTS_HARD_CAP = 100,000`
- **Lines:** [759-762](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L759-L762)
- **Condition:** `cvector_size(ctx.segments) >= 100,000`
- **Trigger:** Very long runs with many valid segments
- **Message:** `"aborting at %d segments (memory safety cap)"`

```c
if (ctx.segments != NULL && (int)cvector_size(ctx.segments) >= BOUNCE_MAX_SEGMENTS_HARD_CAP) {
    printf("bounce_run_pipeline: aborting at %d segments (memory safety cap)\n", ...);
    break;
}
```

### 4. **Angle Selection Fails**
- **Lines:** [638-642](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L638-L642)
- **Function:** `bounce_pick_angle()` returns `BOUNCE_STEP_FAIL`
- **Cause:** Unable to find a valid random angle (rare; usually indicates environment issues)
- **Message:** `"angle selection failed at iteration %d"`

```c
bounce_step_status_t angle_status = bounce_pick_angle(&ctx);
if (angle_status == BOUNCE_STEP_FAIL) {
    printf("bounce_run_pipeline: angle selection failed...\n");
    break;
}
```

### 5. **Ray Casting Fails**
- **Lines:** [645-650](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L645-L650)
- **Function:** `bounce_cast_ray()` returns `BOUNCE_STEP_FAIL`
- **Cause:** Unable to cast a valid ray from current position
- **Message:** `"ray cast failed at iteration %d"`

```c
bounce_step_status_t ray_status = bounce_cast_ray(&ctx, &segment);
if (ray_status == BOUNCE_STEP_FAIL) {
    printf("bounce_run_pipeline: ray cast failed...\n");
    break;
}
```

### 6. **Consecutive Retries Exceeded (Ray Cast)**
- **Constant:** `BOUNCE_MAX_CONSECUTIVE_RETRIES = 32`
- **Lines:** [651-680](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L651-L680)
- **Condition:** `bounce_cast_ray()` returns `BOUNCE_STEP_RETRY` > 32 times consecutively
- **Cause:** Floating-point position drift ("numeric residuals") pushes current position outside boundary
- **Recovery:** Re-snaps to centroid; if that fails, continues with reset angle state
- **Message:** `"too many retries at iter %d — re-snapping position"`

```c
if (ray_status == BOUNCE_STEP_RETRY) {
    ++consecutive_retries;
    if (consecutive_retries > BOUNCE_MAX_CONSECUTIVE_RETRIES) {
        // Re-snap to safe point
        printf("bounce_run_pipeline: too many retries at iter %d — re-snapping...\n", ...);
        ...
        consecutive_retries = 0;
    }
    continue;
}
```

### 7. **Segment Validation Fails (Most Common in Practice)**
- **Lines:** [689-720](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L689-L720)
- **Function:** `bounce_validate_segment_with_reason()` returns false
- **Validates Against:** **Realworld geometry** (original, un-transformed boundary & obstacles)
- **Checks Performed:**
  1. Both segment endpoints inside boundary
  2. Both segment endpoints outside all obstacles
  3. Segment line doesn't cross boundary edges
  4. Segment line doesn't cross into obstacle edges

**Validation Failure Reasons:**
- `BOUNCE_SEGMENT_VALIDATION_START_OUTSIDE_BOUNDARY`
- `BOUNCE_SEGMENT_VALIDATION_END_OUTSIDE_BOUNDARY`
- `BOUNCE_SEGMENT_VALIDATION_START_INSIDE_OBSTACLE`
- `BOUNCE_SEGMENT_VALIDATION_END_INSIDE_OBSTACLE`
- `BOUNCE_SEGMENT_VALIDATION_CROSSES_BOUNDARY_EDGE`
- `BOUNCE_SEGMENT_VALIDATION_CROSSES_OBSTACLE_EDGE`

**Debug Logging:** Validation failures are logged on:
- First 12 attempts ([BOUNCE_VALIDATION_DEBUG_EARLY_LIMIT = 12](src/core/algo_bounce/compute/compute_runner/bounce_runner.c#L34))
- Every 5000th attempt thereafter

**Consecutive Validation Failures:**
- After 32 consecutive failures, re-snaps position and resets angle state
- **Message:** `"too many validation failures at iter %d — re-snapping position"`

```c
if (!waypoint_valid) {
    // Debug logging...
    ++consecutive_retries;
    if (consecutive_retries > BOUNCE_MAX_CONSECUTIVE_RETRIES) {
        printf("bounce_run_pipeline: too many validation failures at iter %d — re-snapping...\n", ...);
        ...
        consecutive_retries = 0;
    }
    continue;
}
```

**Why This Happens:**
- Ray casting uses `environment` geometry (possibly transformed for headland)
- Validation uses `realworld` geometry (original, larger zones)
- If environments differ significantly, many generated segments fail validation
- Results in excessive retries and eventually early exit

---

## Real-World Scenario: When You See Early Exit Without Targets

**Most Likely Cause:** Segment validation failures

**Symptoms:**
1. Run completes with coverage/distance below configured targets
2. Logs show repeated `"segment validation failed"` messages
3. Eventually `"too many retries"` or `"total attempts"` cap

**Why It Happens:**
- **Headland applied:** Environment is narrowed for headland offset, but realworld geometry is original
- **Gap between geometry versions:** Ray casts valid segments in narrower environment, but they violate original boundary/obstacles
- **Validation strictness:** Comprehensive segment check (including line-crossing tests) catches issues endpoint validation misses

**Current Behavior (v1):**
- No automatic restart of pipeline
- Early exit returns partial result with achieved metrics
- Terminal receives `status: "completed"` but coverage/distance below target

**Repository Memory Discrepancy:**
The stored memory states: *"bounce_run_pipeline now restarts full pipeline attempts until at least one configured stop target is reached"* — but the current code shows **no restart logic**. Either:
1. Restart logic moved to a higher layer (check `bounce_check.c` or `api_endpoint.c`)
2. Memory is outdated
3. Restart feature was reverted

---

## Configuration Impact

| Config | Impact |
|--------|--------|
| `target_distance > 0.001` | Early exit if NOT reached before max_iterations |
| `target_coverage > 0.001` | Early exit if NOT reached before max_iterations |
| `max_iterations = 0` | Uses hard cap (100,000 segments) instead of explicit limit |
| No targets configured | **Early exit guaranteed** — never enters true completion state |
| `realworld` geometry missing | Skips validation (uses environment only) — fewer validation failures |

---

## Debugging Checklist

When investigating early exits:

- [ ] Check `max_iterations` value in request
- [ ] Verify `target_distance` and `target_coverage` are configured (> 0.001)
- [ ] Count validation failure messages in logs
- [ ] Check if headland is enabled (increases geometry mismatch risk)
- [ ] Compare environment vs realworld geometry complexity
- [ ] Look for "total attempts" or "segments hard cap" messages
- [ ] Verify realworld geometry is provided when headland is enabled

