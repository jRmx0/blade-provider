/**
 * bounce_runner.h
 *
 * Pipeline context types, step status codes, and the runner entry point for
 * the Bounce algorithm.
 *
 * Shared by bounce_compute.c (orchestrator) and all step modules.
 */

#ifndef BOUNCE_RUNNER_H
#define BOUNCE_RUNNER_H

#include "../../internal.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"

// ---------------------------------------------------------------------------
// Step status
// ---------------------------------------------------------------------------

typedef enum
{
    BOUNCE_STEP_OK = 0, // Step succeeded; proceed.
    BOUNCE_STEP_RETRY,  // Transient failure; caller may retry (e.g. re-pick angle).
    BOUNCE_STEP_FAIL,   // Unrecoverable failure; abort pipeline.
} bounce_step_status_t;

// ---------------------------------------------------------------------------
// Segment (one straight coverage line)
// ---------------------------------------------------------------------------

typedef struct
{
    point_t start;
    point_t end;
} bounce_segment_t;

// ---------------------------------------------------------------------------
// Cumulative metrics
// ---------------------------------------------------------------------------

typedef struct
{
    float total_distance;     // Sum of all segment lengths (same units as environment).
    float estimated_coverage; // Estimated coverage percentage [0, 100].
    int iteration;            // Current loop iteration count.
} bounce_metrics_t;

// ---------------------------------------------------------------------------
// Coverage grid
// ---------------------------------------------------------------------------

typedef struct
{
    float origin_x;         // X of grid origin (boundary bbox min_x)
    float origin_y;         // Y of grid origin (boundary bbox min_y)
    float cell_size;        // Side length of each square cell
    uint32_t cols;          // Number of columns
    uint32_t rows;          // Number of rows
    uint32_t valid_count;   // AOI cells (inside boundary, outside obstacles)
    uint32_t covered_count; // AOI cells hit by at least one segment footprint
    uint8_t *cells;         // cells[row*cols+col]: bit0=valid, bit1=covered
} bounce_coverage_grid_t;

// ---------------------------------------------------------------------------
// Pipeline context
// ---------------------------------------------------------------------------

typedef struct
{
    // --- Input ---
    input_environment_t *original_env; // Pre-validated environment passed to the runner.

    // --- Active environment ---
    input_environment_t *active_env;

    // --- Loop state ---
    point_t current_position; // Start of the next segment.
    float current_angle;      // Direction of travel in radians.
    bool angle_initialized;   // True once an angle has been set.

    // Outward-facing normalized edge normal at the last collision point.
    // Set by bounce_cast_ray(); read by bounce_pick_angle() to compute reflections.
    point_t hit_normal;
    bool has_hit_normal;

    // The edge [A, B] that was hit on the previous ray cast.  bounce_cast_ray
    // skips this edge explicitly so the next ray cannot re-hit the same edge
    // due to the float-lerp residual in current_position (~0.06 m for Mercator
    // coordinates), which would otherwise produce a spurious t >> TMIN in
    // double-precision arithmetic and emit a near-zero backward segment.
    point_t last_hit_edge_A;
    point_t last_hit_edge_B;
    bool has_last_hit_edge;

    // --- Output ---
    cvector_vector_type(bounce_segment_t) segments; // Accumulated coverage segments.

    // --- Metrics ---
    bounce_metrics_t metrics;

    // --- Coverage grid (built lazily on first bounce_update_metrics call) ---
    bounce_coverage_grid_t coverage_grid;
    bool coverage_grid_ready;
} bounce_pipeline_context_t;

// ---------------------------------------------------------------------------
// Runner entry point
// ---------------------------------------------------------------------------

// Runs the full Bounce pipeline on a pre-validated environment.
// Returns a cJSON result object owned by the caller (free with cJSON_Delete).
// Returns NULL only on catastrophic allocation failure.
cJSON *bounce_run_pipeline(input_environment_t *environment);

#endif // BOUNCE_RUNNER_H
