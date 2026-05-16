/**
 * bounce_runner.c
 *
 * Implements the Bounce pipeline loop.
 *
 * Algorithm steps per iteration:
 *   1. Pick a random valid travel angle.
 *   2. Cast a straight ray from the current position until boundary/obstacle collision.
 *   3. Update cumulative path length and coverage estimate.
 *   4. Repeat steps 1-3 until a stop condition is met.
 *   5. Serialize and return the coverage path result.
 *
 * Stop condition: stop when the first configured target is reached among
 * target_distance, target_coverage, and max_iterations.
 *
 * Dependencies: bounce_runner.h, step modules
 */

#include "bounce_runner.h"
#include "../steps/bounce_angle_step.h"
#include "../steps/bounce_ray_step.h"
#include "../steps/bounce_metrics_step.h"
#include "../../../common/debug_serialize.h"

#include <string.h>
#include <stdio.h>
#include <float.h>

#define BOUNCE_MAX_CONSECUTIVE_RETRIES 32
#define BOUNCE_MAX_TOTAL_ATTEMPTS 600000
#define BOUNCE_MAX_SEGMENTS_HARD_CAP 100000
#define BOUNCE_VALIDATION_DEBUG_EARLY_LIMIT 12
#define BOUNCE_VALIDATION_DEBUG_PERIOD 5000

// ---------------------------------------------------------------------------
// Context lifecycle
// ---------------------------------------------------------------------------

static void bounce_context_init(bounce_pipeline_context_t *ctx, input_environment_t *env)
{
    memset(ctx, 0, sizeof(bounce_pipeline_context_t));
    ctx->original_env = env;
    ctx->active_env = env;
    ctx->angle_initialized = false;
    ctx->current_position = env->start_point;
    ctx->segments = NULL;
    ctx->metrics.total_distance = 0.0f;
    ctx->metrics.estimated_coverage = 0.0f;
    ctx->metrics.iteration = 0;
}

static void bounce_context_free(bounce_pipeline_context_t *ctx)
{
    if (ctx->segments != NULL)
    {
        cvector_free(ctx->segments);
        ctx->segments = NULL;
    }
    if (ctx->coverage_grid.cells != NULL)
    {
        va_free(ctx->coverage_grid.cells);
        ctx->coverage_grid.cells = NULL;
    }
}

// ---------------------------------------------------------------------------
// Geometry utilities
// ---------------------------------------------------------------------------

/**
 * Ray-casting point-in-polygon test.
 * Casts a horizontal ray from p in the +X direction and counts boundary
 * crossings. Returns true when the crossing count is odd (point is inside).
 *
 * Edge cases: points exactly on an edge are treated as inside because the
 * algorithm uses strict < for the lower bound and <= for the upper bound on
 * Y, which is the standard "top-left" fill convention.
 */
static bool bounce_point_in_polygon(point_t p, const polygon_t *polygon)
{
    if (polygon->edges == NULL || polygon->edge_count == 0)
    {
        return false;
    }

    int crossings = 0;
    for (uint32_t i = 0; i < polygon->edge_count; ++i)
    {
        point_t A = polygon->edges[i].begin;
        point_t B = polygon->edges[i].end;

        float minY = A.y < B.y ? A.y : B.y;
        float maxY = A.y > B.y ? A.y : B.y;

        // The horizontal ray at p.y must cross strictly inside the y-span of
        // the edge to avoid double-counting shared vertices.
        if (p.y <= minY || p.y > maxY)
        {
            continue;
        }

        // X coordinate of the edge at y == p.y via linear interpolation.
        float t = (p.y - A.y) / (B.y - A.y);
        float xEdge = A.x + t * (B.x - A.x);

        if (xEdge > p.x)
        {
            ++crossings;
        }
    }

    return (crossings & 1) == 1;
}

/**
 * Returns true if p is inside the active environment: inside the boundary
 * polygon AND outside every obstacle polygon.
 */
static bool bounce_point_in_active_env(point_t p, const input_environment_t *env)
{
    if (!bounce_point_in_polygon(p, &env->boundary))
    {
        return false;
    }
    for (uint32_t k = 0; k < env->obstacle_count; ++k)
    {
        if (bounce_point_in_polygon(p, &env->obstacles[k]))
        {
            return false;
        }
    }
    return true;
}

/**
 * Returns true if p is inside the provided geometry set: inside boundary
 * polygon AND outside every obstacle polygon.
 */
static bool bounce_point_in_geometry(point_t p,
                                     const polygon_t *boundary,
                                     const polygon_t *obstacles,
                                     uint32_t obstacle_count)
{
    if (boundary == NULL || !bounce_point_in_polygon(p, boundary))
    {
        return false;
    }
    for (uint32_t k = 0; k < obstacle_count; ++k)
    {
        if (obstacles != NULL && bounce_point_in_polygon(p, &obstacles[k]))
        {
            return false;
        }
    }
    return true;
}

/**
 * Simple arithmetic centroid of a polygon's vertices.
 * Always produces a finite point; for convex polygons it is guaranteed to be
 * inside the polygon.
 */
static point_t bounce_polygon_centroid(const polygon_t *polygon)
{
    float cx = 0.0f;
    float cy = 0.0f;

    if (polygon->vertices == NULL || polygon->vertex_count == 0)
    {
        return (point_t){0.0f, 0.0f};
    }

    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        cx += polygon->vertices[i].x;
        cy += polygon->vertices[i].y;
    }
    cx /= (float)polygon->vertex_count;
    cy /= (float)polygon->vertex_count;
    return (point_t){cx, cy};
}

/**
 * Finds a safe point in the active environment (inside boundary and outside
 * obstacles), preferring points near `preferred`.
 *
 * Strategy:
 *   1) If preferred is already safe, return it.
 *   2) Grid-sample boundary bounding box (coarse then fine) and pick the
 *      closest safe sample to preferred.
 */
static bool bounce_find_safe_point_in_active_env(const input_environment_t *env,
                                                 point_t preferred,
                                                 point_t *out_point)
{
    if (env == NULL || out_point == NULL || env->boundary.vertices == NULL || env->boundary.vertex_count < 3)
    {
        return false;
    }

    if (bounce_point_in_active_env(preferred, env))
    {
        *out_point = preferred;
        return true;
    }

    float min_x = env->boundary.vertices[0].x;
    float max_x = min_x;
    float min_y = env->boundary.vertices[0].y;
    float max_y = min_y;
    for (uint32_t i = 1; i < env->boundary.vertex_count; ++i)
    {
        float x = env->boundary.vertices[i].x;
        float y = env->boundary.vertices[i].y;
        if (x < min_x)
            min_x = x;
        if (x > max_x)
            max_x = x;
        if (y < min_y)
            min_y = y;
        if (y > max_y)
            max_y = y;
    }

    const int passes[2] = {31, 61};
    float best_d2 = FLT_MAX;
    point_t best = preferred;
    bool found = false;

    for (int p = 0; p < 2; ++p)
    {
        int steps = passes[p];
        float dx = (max_x - min_x) / (float)steps;
        float dy = (max_y - min_y) / (float)steps;
        if (dx < 1e-3f)
            dx = 1.0f;
        if (dy < 1e-3f)
            dy = 1.0f;

        for (int iy = 0; iy <= steps; ++iy)
        {
            for (int ix = 0; ix <= steps; ++ix)
            {
                point_t c = {min_x + (float)ix * dx, min_y + (float)iy * dy};
                if (!bounce_point_in_active_env(c, env))
                {
                    continue;
                }

                float ddx = c.x - preferred.x;
                float ddy = c.y - preferred.y;
                float d2 = ddx * ddx + ddy * ddy;
                if (!found || d2 < best_d2)
                {
                    best = c;
                    best_d2 = d2;
                    found = true;
                }
            }
        }

        if (found)
        {
            *out_point = best;
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Segment validation against realworld geometry
// ---------------------------------------------------------------------------

/**
 * Check if two line segments intersect in their interiors (not at endpoints).
 *
 * Segment 1: A + s*(B-A), s in [0,1]
 * Segment 2 (edge): P + t*(Q-P), t in [0,1]
 *
 * Uses parametric form and double-precision cross products to avoid
 * catastrophic cancellation on large coordinates.
 *
 * Returns true if segments intersect in their interiors.
 */
static bool segment_intersects_edge(point_t A, point_t B, point_t P, point_t Q)
{
    double ABx = (double)B.x - (double)A.x;
    double ABy = (double)B.y - (double)A.y;

    double PQx = (double)Q.x - (double)P.x;
    double PQy = (double)Q.y - (double)P.y;

    double APx = (double)P.x - (double)A.x;
    double APy = (double)P.y - (double)A.y;

    // Cross products for parametric solution
    double denom = ABx * PQy - ABy * PQx;
    if (denom < 1e-12 && denom > -1e-12)
    {
        return false; // Segments are parallel or collinear
    }

    // Solve for s (parameter on segment AB) and t (parameter on edge PQ)
    double s = (APx * PQy - APy * PQx) / denom;
    double t = (APx * ABy - APy * ABx) / denom;

    // Intersection in interior of both segments (excluding endpoints)
    // Using strict inequalities: 0 < s < 1 and 0 < t < 1
    // This avoids false positives at shared vertices.
    return s > 1e-6 && s < 1.0 - 1e-6 && t > 1e-6 && t < 1.0 - 1e-6;
}

typedef enum
{
    BOUNCE_SEGMENT_VALIDATION_OK = 0,
    BOUNCE_SEGMENT_VALIDATION_INVALID_GEOMETRY,
    BOUNCE_SEGMENT_VALIDATION_START_OUTSIDE_BOUNDARY,
    BOUNCE_SEGMENT_VALIDATION_END_OUTSIDE_BOUNDARY,
    BOUNCE_SEGMENT_VALIDATION_START_INSIDE_OBSTACLE,
    BOUNCE_SEGMENT_VALIDATION_END_INSIDE_OBSTACLE,
    BOUNCE_SEGMENT_VALIDATION_CROSSES_BOUNDARY_EDGE,
    BOUNCE_SEGMENT_VALIDATION_CROSSES_OBSTACLE_EDGE,
} bounce_segment_validation_reason_t;

static const char *bounce_segment_validation_reason_str(bounce_segment_validation_reason_t reason)
{
    switch (reason)
    {
    case BOUNCE_SEGMENT_VALIDATION_OK:
        return "ok";
    case BOUNCE_SEGMENT_VALIDATION_INVALID_GEOMETRY:
        return "invalid_geometry";
    case BOUNCE_SEGMENT_VALIDATION_START_OUTSIDE_BOUNDARY:
        return "start_outside_boundary";
    case BOUNCE_SEGMENT_VALIDATION_END_OUTSIDE_BOUNDARY:
        return "end_outside_boundary";
    case BOUNCE_SEGMENT_VALIDATION_START_INSIDE_OBSTACLE:
        return "start_inside_obstacle";
    case BOUNCE_SEGMENT_VALIDATION_END_INSIDE_OBSTACLE:
        return "end_inside_obstacle";
    case BOUNCE_SEGMENT_VALIDATION_CROSSES_BOUNDARY_EDGE:
        return "crosses_boundary_edge";
    case BOUNCE_SEGMENT_VALIDATION_CROSSES_OBSTACLE_EDGE:
        return "crosses_obstacle_edge";
    default:
        return "unknown";
    }
}

static bool bounce_validate_segment_with_reason(
    point_t p1, point_t p2,
    const polygon_t *boundary,
    const polygon_t *obstacles,
    uint32_t obstacle_count,
    bounce_segment_validation_reason_t *reason_out,
    uint32_t *obstacle_index_out,
    uint32_t *edge_index_out)
{
    if (reason_out != NULL)
        *reason_out = BOUNCE_SEGMENT_VALIDATION_OK;
    if (obstacle_index_out != NULL)
        *obstacle_index_out = UINT32_MAX;
    if (edge_index_out != NULL)
        *edge_index_out = UINT32_MAX;

    if (boundary == NULL || boundary->vertices == NULL || boundary->vertex_count < 3 || boundary->edges == NULL)
    {
        if (reason_out != NULL)
            *reason_out = BOUNCE_SEGMENT_VALIDATION_INVALID_GEOMETRY;
        return false;
    }

    if (!bounce_point_in_polygon(p1, boundary))
    {
        if (reason_out != NULL)
            *reason_out = BOUNCE_SEGMENT_VALIDATION_START_OUTSIDE_BOUNDARY;
        return false;
    }

    if (!bounce_point_in_polygon(p2, boundary))
    {
        if (reason_out != NULL)
            *reason_out = BOUNCE_SEGMENT_VALIDATION_END_OUTSIDE_BOUNDARY;
        return false;
    }

    for (uint32_t k = 0; k < obstacle_count; ++k)
    {
        if (obstacles == NULL || obstacles[k].vertices == NULL || obstacles[k].vertex_count < 3)
        {
            continue;
        }
        if (bounce_point_in_polygon(p1, &obstacles[k]))
        {
            if (reason_out != NULL)
                *reason_out = BOUNCE_SEGMENT_VALIDATION_START_INSIDE_OBSTACLE;
            if (obstacle_index_out != NULL)
                *obstacle_index_out = k;
            return false;
        }
        if (bounce_point_in_polygon(p2, &obstacles[k]))
        {
            if (reason_out != NULL)
                *reason_out = BOUNCE_SEGMENT_VALIDATION_END_INSIDE_OBSTACLE;
            if (obstacle_index_out != NULL)
                *obstacle_index_out = k;
            return false;
        }
    }

    for (uint32_t i = 0; i < boundary->edge_count; ++i)
    {
        if (segment_intersects_edge(p1, p2,
                                    boundary->edges[i].begin,
                                    boundary->edges[i].end))
        {
            if (reason_out != NULL)
                *reason_out = BOUNCE_SEGMENT_VALIDATION_CROSSES_BOUNDARY_EDGE;
            if (edge_index_out != NULL)
                *edge_index_out = i;
            return false;
        }
    }

    for (uint32_t k = 0; k < obstacle_count; ++k)
    {
        if (obstacles == NULL || obstacles[k].vertices == NULL || obstacles[k].vertex_count < 3 || obstacles[k].edges == NULL)
        {
            continue;
        }
        for (uint32_t i = 0; i < obstacles[k].edge_count; ++i)
        {
            if (segment_intersects_edge(p1, p2,
                                        obstacles[k].edges[i].begin,
                                        obstacles[k].edges[i].end))
            {
                if (reason_out != NULL)
                    *reason_out = BOUNCE_SEGMENT_VALIDATION_CROSSES_OBSTACLE_EDGE;
                if (obstacle_index_out != NULL)
                    *obstacle_index_out = k;
                if (edge_index_out != NULL)
                    *edge_index_out = i;
                return false;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Stop condition
// ---------------------------------------------------------------------------

static bool bounce_targets_reached(const bounce_pipeline_context_t *ctx,
                                   const input_environment_t *env)
{
    if (env->target_distance > 0.001f && ctx->metrics.total_distance >= env->target_distance)
    {
        return true;
    }
    if (env->target_coverage > 0.001f && ctx->metrics.estimated_coverage >= env->target_coverage)
    {
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Error result serialization
// ---------------------------------------------------------------------------

/**
 * Create an error result JSON indicating early exit without hitting targets.
 *
 * Returns a JSON object with status: "error" so it can be detected at the
 * API level and converted to a proper error response.
 */
static cJSON *bounce_serialize_early_exit_error(
    const bounce_pipeline_context_t *ctx,
    const input_environment_t *env)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "code", "early_exit");

    // Build descriptive message
    char message[512];
    snprintf(message, sizeof(message),
             "Bounce exited early without reaching targets. "
             "Achieved: distance=%.2f/%s, coverage=%.1f%%/%s, iterations=%d. "
             "Configured targets: distance_target=%.2f, coverage_target=%.1f%%, max_iterations=%u",
             ctx->metrics.total_distance,
             (env->target_distance > 0.001f) ? "required" : "unconfigured",
             ctx->metrics.estimated_coverage * 100.0f,
             (env->target_coverage > 0.001f) ? "required" : "unconfigured",
             ctx->metrics.iteration,
             env->target_distance,
             env->target_coverage * 100.0f,
             env->max_iterations);

    cJSON_AddStringToObject(root, "message", message);

    // Include partial metrics for debugging
    cJSON *metrics = cJSON_CreateObject();
    if (metrics != NULL)
    {
        cJSON_AddNumberToObject(metrics, "total_distance", ctx->metrics.total_distance);
        cJSON_AddNumberToObject(metrics, "estimated_coverage", ctx->metrics.estimated_coverage);
        cJSON_AddNumberToObject(metrics, "iterations", ctx->metrics.iteration);
        cJSON_AddItemToObject(root, "partialMetrics", metrics);
    }

    return root;
}

// ---------------------------------------------------------------------------
// Result serialization
// ---------------------------------------------------------------------------

static cJSON *bounce_serialize_result(const bounce_pipeline_context_t *ctx)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    // --- coveragePathPlan ---
    cJSON *coverage_path_plan = cJSON_CreateObject();
    cJSON *segments_arr = cJSON_CreateArray();
    if (coverage_path_plan == NULL || segments_arr == NULL)
    {
        cJSON_Delete(root);
        cJSON_Delete(coverage_path_plan);
        cJSON_Delete(segments_arr);
        return NULL;
    }
    cJSON_AddItemToObject(coverage_path_plan, "segments", segments_arr);
    cJSON_AddItemToObject(root, "coveragePathPlan", coverage_path_plan);

    int segment_count = (ctx->segments != NULL) ? (int)cvector_size(ctx->segments) : 0;
    for (int i = 0; i < segment_count; ++i)
    {
        const bounce_segment_t *seg = &ctx->segments[i];
        float xs[2] = {seg->start.x, seg->end.x};
        float ys[2] = {seg->start.y, seg->end.y};
        cJSON *jseg = debug_build_segment(i, "coverage", xs, ys, 2);
        if (jseg != NULL)
        {
            cJSON_AddItemToArray(segments_arr, jseg);
        }
    }

    // --- debug ---
    cJSON *debug = cJSON_CreateObject();
    cJSON *debug_layers = cJSON_CreateArray();
    if (debug != NULL && debug_layers != NULL)
    {
        cJSON_AddItemToObject(debug, "layers", debug_layers);
        cJSON_AddItemToObject(root, "debug", debug);
    }

    return root;
}

// ---------------------------------------------------------------------------
// Pipeline entry point
// ---------------------------------------------------------------------------

cJSON *bounce_run_pipeline(input_environment_t *environment)
{
    bounce_pipeline_context_t ctx;
    bounce_context_init(&ctx, environment);
    uint32_t max_iterations = environment->max_iterations;

    // Validate that the starting position is inside active environment
    // (inside boundary and outside obstacles). If not, snap to a safe point.
    if (!bounce_point_in_active_env(ctx.current_position, ctx.active_env))
    {
        point_t preferred = bounce_polygon_centroid(&ctx.active_env->boundary);
        point_t safe = preferred;
        if (bounce_find_safe_point_in_active_env(ctx.active_env, preferred, &safe))
        {
            printf("bounce_run_pipeline: start_point invalid (outside boundary or inside obstacle) — snapping to safe point (%.2f, %.2f)\n",
                   safe.x, safe.y);
            ctx.current_position = safe;
        }
        else
        {
            printf("bounce_run_pipeline: unable to find safe start point in environment\n");
        }
    }

    // --- Steps 2-5: Main loop ---
    // Safety caps:
    // - consecutive retries: recover from local numeric drift
    // - total attempts: prevents endless retry loops from running forever
    // - segment hard cap (when max_iterations is unset): bounds peak memory use
    int consecutive_retries = 0;
    int total_attempts = 0;

    while (!bounce_targets_reached(&ctx, environment) &&
           ((max_iterations == 0u)
                ? (ctx.metrics.iteration < BOUNCE_MAX_SEGMENTS_HARD_CAP)
                : (ctx.metrics.iteration < (int)max_iterations)))
    {
        ++total_attempts;
        if (total_attempts > BOUNCE_MAX_TOTAL_ATTEMPTS)
        {
            printf("bounce_run_pipeline: aborting after %d total attempts (memory/loop safety cap)\n",
                   total_attempts);
            break;
        }

        // NOTE: ctx.metrics.iteration is incremented only when a segment is
        // successfully produced below. Retries must not consume the budget.

        // --- Step 2: Pick random valid travel angle ---
        bounce_step_status_t angle_status = bounce_pick_angle(&ctx);
        if (angle_status == BOUNCE_STEP_FAIL)
        {
            printf("bounce_run_pipeline: angle selection failed at iteration %d\n",
                   ctx.metrics.iteration);
            break;
        }

        // --- Step 3: Cast ray until collision ---
        bounce_segment_t segment;
        bounce_step_status_t ray_status = bounce_cast_ray(&ctx, &segment);
        if (ray_status == BOUNCE_STEP_FAIL)
        {
            printf("bounce_run_pipeline: ray cast failed at iteration %d\n",
                   ctx.metrics.iteration);
            break;
        }
        if (ray_status == BOUNCE_STEP_RETRY)
        {
            ++consecutive_retries;
            if (consecutive_retries > BOUNCE_MAX_CONSECUTIVE_RETRIES)
            {
                // Origin has drifted outside the boundary due to float-lerp residuals.
                // Snap back to a known-good interior point and reset angle state
                // so the next pick starts fresh rather than reflecting off a
                // stale normal that may point outward.
                printf("bounce_run_pipeline: too many retries at iter %d — re-snapping position\n",
                       ctx.metrics.iteration);
                point_t preferred = bounce_polygon_centroid(&ctx.active_env->boundary);
                point_t safe = preferred;
                if (bounce_find_safe_point_in_active_env(ctx.active_env, preferred, &safe))
                {
                    ctx.current_position = safe;
                    printf("bounce_run_pipeline: re-snapped to safe point (%.2f, %.2f)\n",
                           safe.x, safe.y);
                }
                ctx.has_hit_normal = false;
                consecutive_retries = 0;
            }
            continue;
        }

        consecutive_retries = 0;

        // --- Step 3b: Validate generated segment stays inside realworld geometry ---
        // Validates that the entire segment (not just endpoints) stays within realworld
        // geometry. Realworld geometry is the original, un-transformed zone boundary
        // and obstacles. This comprehensive check ensures:
        //   1. Both endpoints are inside the boundary
        //   2. Both endpoints are outside all obstacles
        //   3. The segment line doesn't cross the boundary (stays inside zone)
        //   4. The segment line doesn't cross into obstacles
        //
        // Without this check, segments can pass between endpoint-validation but still
        // traverse outside the zone when the segment crosses a concave or inward-facing
        // boundary section. This is especially common when realworld and environment
        // geometries differ (environment is transformed/narrower).
        bool waypoint_valid = false;
        bounce_segment_validation_reason_t validation_reason = BOUNCE_SEGMENT_VALIDATION_OK;
        uint32_t validation_obstacle_idx = UINT32_MAX;
        uint32_t validation_edge_idx = UINT32_MAX;

        // Validate segment against realworld geometry (original, larger boundary).
        waypoint_valid = bounce_validate_segment_with_reason(
            segment.start, segment.end,
            &ctx.active_env->realworld_boundary,
            ctx.active_env->realworld_obstacles,
            ctx.active_env->realworld_obstacle_count,
            &validation_reason, &validation_obstacle_idx, &validation_edge_idx);

        if (!waypoint_valid)
        {
            bool print_debug =
                (total_attempts <= BOUNCE_VALIDATION_DEBUG_EARLY_LIMIT) ||
                (total_attempts % BOUNCE_VALIDATION_DEBUG_PERIOD == 0);

            if (print_debug)
            {
                int obs_idx_print = (validation_obstacle_idx == UINT32_MAX) ? -1 : (int)validation_obstacle_idx;
                int edge_idx_print = (validation_edge_idx == UINT32_MAX) ? -1 : (int)validation_edge_idx;
                printf("bounce_run_pipeline: segment validation failed at iter %d attempt %d reason=%s start=(%.3f,%.3f) end=(%.3f,%.3f) angle=%.6f obs_idx=%d edge_idx=%d\n",
                       ctx.metrics.iteration,
                       total_attempts,
                       bounce_segment_validation_reason_str(validation_reason),
                       segment.start.x,
                       segment.start.y,
                       segment.end.x,
                       segment.end.y,
                       ctx.current_angle,
                       obs_idx_print,
                       edge_idx_print);
            }

            ++consecutive_retries;
            if (consecutive_retries > BOUNCE_MAX_CONSECUTIVE_RETRIES)
            {
                // Too many validation failures indicate position has drifted.
                // Re-snap to a known-good interior point and reset angle state.
                printf("bounce_run_pipeline: too many validation failures at iter %d — re-snapping position\n",
                       ctx.metrics.iteration);
                point_t preferred = bounce_polygon_centroid(&ctx.active_env->boundary);
                point_t safe = preferred;
                if (bounce_find_safe_point_in_active_env(ctx.active_env, preferred, &safe))
                {
                    ctx.current_position = safe;
                    printf("bounce_run_pipeline: validation re-snap to safe point (%.2f, %.2f)\n",
                           safe.x, safe.y);
                }
                ctx.has_hit_normal = false;
                consecutive_retries = 0;
            }
            continue;
        }

        // Commit segment and advance current position to the collision point.
        // Only now does the iteration count increment: max_iterations counts
        // produced segments, not retry attempts.
        if (ctx.segments != NULL && (int)cvector_size(ctx.segments) >= BOUNCE_MAX_SEGMENTS_HARD_CAP)
        {
            printf("bounce_run_pipeline: aborting at %d segments (memory safety cap)\n",
                   (int)cvector_size(ctx.segments));
            break;
        }

        ctx.metrics.iteration++;
        cvector_push_back(ctx.segments, segment);
        ctx.current_position = segment.end;

        // --- Step 4: Update cumulative metrics ---
        bounce_update_metrics(&ctx, &segment);

        printf("bounce_run_pipeline: iter %d — dist=%.2f cov=%.2f%%\n",
               ctx.metrics.iteration,
               ctx.metrics.total_distance,
               ctx.metrics.estimated_coverage);
    }

    printf("bounce_run_pipeline: loop ended after %d iter(s), %d segment(s)\n",
           ctx.metrics.iteration,
           (ctx.segments != NULL) ? (int)cvector_size(ctx.segments) : 0);

    // Check if targets were actually reached
    bool targets_reached = bounce_targets_reached(&ctx, environment);

    if (!targets_reached)
    {
        printf("bounce_run_pipeline: early exit — targets not reached\n");
        cJSON *error_result = bounce_serialize_early_exit_error(&ctx, environment);
        bounce_context_free(&ctx);
        return error_result;
    }

    // --- Step 6: Serialize result ---
    cJSON *result = bounce_serialize_result(&ctx);
    bounce_context_free(&ctx);
    return result;
}
