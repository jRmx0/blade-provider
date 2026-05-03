/**
 * bounce_runner.c
 *
 * Implements the Bounce pipeline loop.
 *
 * Algorithm steps per iteration:
 *   1. (Once) Apply headland conversion when environment->headland == true.
 *   2. Pick a random valid travel angle.
 *   3. Cast a straight ray from the current position until boundary/obstacle collision.
 *   4. Update cumulative path length and coverage estimate.
 *   5. Repeat steps 2-4 until a stop condition is met.
 *   6. Serialize and return the coverage path result.
 *
 * Stop condition: stop when EITHER target_distance OR target_coverage is reached
 * (whichever comes first). A max-iteration guard prevents infinite loops while
 * step implementations are still stubbed.
 *
 * Dependencies: bounce_runner.h, step modules
 */

#include "bounce_runner.h"
#include "../steps/bounce_headland_step.h"
#include "../steps/bounce_angle_step.h"
#include "../steps/bounce_ray_step.h"
#include "../steps/bounce_metrics_step.h"

#include <string.h>
#include <stdio.h>

#define BOUNCE_MAX_ITERATIONS 10000

// ---------------------------------------------------------------------------
// Context lifecycle
// ---------------------------------------------------------------------------

static void bounce_context_init(bounce_pipeline_context_t *ctx, input_environment_t *env)
{
    memset(ctx, 0, sizeof(bounce_pipeline_context_t));
    ctx->original_env = env;
    ctx->active_env = env;
    ctx->has_headland = false;
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
    if (ctx->has_headland)
    {
        free_headland(&ctx->headland);
        ctx->has_headland = false;
    }
    if (ctx->coverage_grid.cells != NULL)
    {
        va_free(ctx->coverage_grid.cells);
        ctx->coverage_grid.cells = NULL;
    }
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

    // Headland debug layers (populated only when headland was applied)
    if (ctx->has_headland && debug_layers != NULL)
    {
        // Shrunken zone border (id=13)
        {
            cJSON *layer = cJSON_CreateObject();
            if (layer != NULL)
            {
                cJSON_AddNumberToObject(layer, "id", 13);
                cJSON_AddStringToObject(layer, "source", "headlandShrunkenZoneBorder");
                cJSON *list = cJSON_CreateArray();
                if (list != NULL)
                {
                    if (ctx->headland.shrunken_zone.vertices != NULL &&
                        ctx->headland.shrunken_zone.vertex_count > 0)
                    {
                        cJSON *entry = cJSON_CreateObject();
                        cJSON *vertices = cJSON_CreateArray();
                        if (entry != NULL && vertices != NULL)
                        {
                            cJSON_AddNumberToObject(entry, "id", 1);
                            for (uint32_t j = 0; j < ctx->headland.shrunken_zone.vertex_count; ++j)
                            {
                                cJSON *jpt = cJSON_CreateObject();
                                if (jpt != NULL)
                                {
                                    cJSON_AddNumberToObject(jpt, "x", ctx->headland.shrunken_zone.vertices[j].x);
                                    cJSON_AddNumberToObject(jpt, "y", ctx->headland.shrunken_zone.vertices[j].y);
                                    cJSON_AddItemToArray(vertices, jpt);
                                }
                            }
                            cJSON_AddItemToObject(entry, "vertices", vertices);
                            cJSON_AddItemToArray(list, entry);
                        }
                    }
                    cJSON_AddItemToObject(layer, "list", list);
                }
                cJSON_AddItemToArray(debug_layers, layer);
            }
        }

        // Expanded obstacle borders (id=14)
        {
            cJSON *layer = cJSON_CreateObject();
            if (layer != NULL)
            {
                cJSON_AddNumberToObject(layer, "id", 14);
                cJSON_AddStringToObject(layer, "source", "headlandExpandedObstacleBorders");
                cJSON *list = cJSON_CreateArray();
                if (list != NULL)
                {
                    for (uint32_t k = 0; k < ctx->headland.expanded_obstacle_count; ++k)
                    {
                        const polygon_t *obs = &ctx->headland.expanded_obstacles[k];
                        if (obs->vertices == NULL || obs->vertex_count == 0)
                            continue;

                        cJSON *entry = cJSON_CreateObject();
                        cJSON *vertices = cJSON_CreateArray();
                        if (entry != NULL && vertices != NULL)
                        {
                            cJSON_AddNumberToObject(entry, "id", (double)(k + 1));
                            for (uint32_t j = 0; j < obs->vertex_count; ++j)
                            {
                                cJSON *jpt = cJSON_CreateObject();
                                if (jpt != NULL)
                                {
                                    cJSON_AddNumberToObject(jpt, "x", obs->vertices[j].x);
                                    cJSON_AddNumberToObject(jpt, "y", obs->vertices[j].y);
                                    cJSON_AddItemToArray(vertices, jpt);
                                }
                            }
                            cJSON_AddItemToObject(entry, "vertices", vertices);
                            cJSON_AddItemToArray(list, entry);
                        }
                    }
                    cJSON_AddItemToObject(layer, "list", list);
                }
                cJSON_AddItemToArray(debug_layers, layer);
            }
        }
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

    // --- Step 1: Apply headland (once, before the loop) ---
    bounce_step_status_t hl_status = bounce_apply_headland(environment, &ctx);
    if (hl_status == BOUNCE_STEP_FAIL)
    {
        bounce_context_free(&ctx);
        cJSON *err = cJSON_CreateObject();
        if (err != NULL)
        {
            cJSON_AddStringToObject(err, "status", "error");
            cJSON_AddStringToObject(err, "code", "headland_failed");
            cJSON_AddStringToObject(err, "message", "Bounce headland conversion failed.");
        }
        return err;
    }

    printf("bounce_run_pipeline: headland=%s, start=(%.2f, %.2f)\n",
           ctx.has_headland ? "applied" : "skipped",
           ctx.current_position.x,
           ctx.current_position.y);

    // --- Steps 2-5: Main loop ---
    while (!bounce_targets_reached(&ctx, environment) &&
           ctx.metrics.iteration < BOUNCE_MAX_ITERATIONS)
    {
        ctx.metrics.iteration++;

        // --- Step 2: Pick random valid travel angle ---
        bounce_step_status_t angle_status = bounce_pick_angle(&ctx);
        if (angle_status == BOUNCE_STEP_FAIL)
        {
            printf("bounce_run_pipeline: angle selection failed at iteration %d\n",
                   ctx.metrics.iteration);
            break;
        }
        if (angle_status == BOUNCE_STEP_RETRY)
        {
            continue;
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
            continue;
        }

        // Commit segment and advance current position to the collision point
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
