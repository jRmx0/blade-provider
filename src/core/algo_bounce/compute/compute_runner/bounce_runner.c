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

#include <string.h>
#include <stdio.h>

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
    if (segment_count > 0)
    {
        cJSON *jsegment = cJSON_CreateObject();
        cJSON *path_arr = cJSON_CreateArray();
        if (jsegment != NULL && path_arr != NULL)
        {
            cJSON_AddNumberToObject(jsegment, "id", 1);
            cJSON_AddStringToObject(jsegment, "type", "coverage");
            cJSON_AddItemToObject(jsegment, "path", path_arr);

            // First waypoint = start of the first computed segment.
            const bounce_segment_t *first = &ctx->segments[0];
            cJSON *jstart_entry = cJSON_CreateObject();
            cJSON *jstart_point = cJSON_CreateObject();
            if (jstart_entry != NULL && jstart_point != NULL)
            {
                cJSON_AddNumberToObject(jstart_entry, "id", 1);
                cJSON_AddNumberToObject(jstart_point, "x", first->start.x);
                cJSON_AddNumberToObject(jstart_point, "y", first->start.y);
                cJSON_AddItemToObject(jstart_entry, "point", jstart_point);
                cJSON_AddItemToArray(path_arr, jstart_entry);
            }

            // Next waypoints = end point of each bounce segment in order.
            for (int i = 0; i < segment_count; ++i)
            {
                const bounce_segment_t *seg = &ctx->segments[i];
                cJSON *jentry = cJSON_CreateObject();
                cJSON *jpoint = cJSON_CreateObject();
                if (jentry == NULL || jpoint == NULL)
                {
                    cJSON_Delete(jentry);
                    cJSON_Delete(jpoint);
                    continue;
                }

                cJSON_AddNumberToObject(jentry, "id", i + 2);
                cJSON_AddNumberToObject(jpoint, "x", seg->end.x);
                cJSON_AddNumberToObject(jpoint, "y", seg->end.y);
                cJSON_AddItemToObject(jentry, "point", jpoint);
                cJSON_AddItemToArray(path_arr, jentry);
            }

            cJSON_AddItemToArray(segments_arr, jsegment);
        }
        else
        {
            cJSON_Delete(jsegment);
            cJSON_Delete(path_arr);
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

    // Validate that the starting position is inside the boundary.
    // If the caller provided a start_point outside the boundary,
    // snap to the centroid so that all subsequent ray casts originate
    // from a known-good interior position.
    if (!bounce_point_in_active_env(ctx.current_position, ctx.active_env))
    {
        point_t centroid = bounce_polygon_centroid(&ctx.active_env->boundary);
        printf("bounce_run_pipeline: start_point outside boundary — snapping to centroid (%.2f, %.2f)\n",
               centroid.x, centroid.y);
        ctx.current_position = centroid;
    }

    // --- Steps 2-5: Main loop ---
    // Safety cap for consecutive failed ray casts (e.g. origin drifted outside
    // due to ORIGIN_BIAS nudge near a wall). When hit, snap back to the active
    // env centroid and reset the hit-normal so the next angle pick is fresh.
#define BOUNCE_MAX_CONSECUTIVE_RETRIES 32
    int consecutive_retries = 0;

    while (!bounce_targets_reached(&ctx, environment) &&
           (max_iterations == 0u || ctx.metrics.iteration < (int)max_iterations))
    {
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
                ctx.current_position = bounce_polygon_centroid(&ctx.active_env->boundary);
                ctx.has_hit_normal = false;
                consecutive_retries = 0;
            }
            continue;
        }

        consecutive_retries = 0;

        // Commit segment and advance current position to the collision point.
        // Only now does the iteration count increment: max_iterations counts
        // produced segments, not retry attempts.
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

    // --- Step 6: Serialize result ---
    cJSON *result = bounce_serialize_result(&ctx);
    bounce_context_free(&ctx);
    return result;
}
