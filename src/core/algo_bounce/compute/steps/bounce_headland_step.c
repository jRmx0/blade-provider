/**
 * bounce_headland_step.c
 *
 * Step 1: Applies optional headland conversion to the pipeline context.
 * When headland is enabled, the active environment is replaced with the
 * shrunken-zone geometry produced by compute_headland().
 *
 * The headland_env shallow wrapper mirrors the BCD runner pattern:
 * it points at headland.shrunken_zone and headland.expanded_obstacles
 * without copying the polygon data.
 *
 * Dependencies: bounce_headland_step.h, common/headland.h
 */

#include "bounce_headland_step.h"

#include <string.h>
#include <stdio.h>

bounce_step_status_t bounce_apply_headland(
    input_environment_t *env,
    bounce_pipeline_context_t *ctx)
{
    if (!env->headland)
    {
        ctx->active_env = env;
        ctx->has_headland = false;
        return BOUNCE_STEP_OK;
    }

    memset(&ctx->headland, 0, sizeof(headland_t));
    memset(&ctx->headland_env, 0, sizeof(input_environment_t));

    int rc = compute_headland(env, &ctx->headland);
    if (rc != 0)
    {
        printf("bounce_apply_headland: compute_headland failed (code %d)\n", rc);
        return BOUNCE_STEP_FAIL;
    }

    // Build a shallow environment wrapper pointing at headland geometry,
    // mirroring the headland_env pattern used in BCD's coverage runner.
    ctx->headland_env.id = env->id;
    ctx->headland_env.path_width = env->path_width;
    ctx->headland_env.path_overlap = env->path_overlap;
    ctx->headland_env.bounce_offset = env->bounce_offset;
    ctx->headland_env.target_coverage = env->target_coverage;
    ctx->headland_env.target_distance = env->target_distance;
    ctx->headland_env.track_memory_usage = env->track_memory_usage;
    ctx->headland_env.headland = false; // already processed
    ctx->headland_env.headland_coverage_offset = env->headland_coverage_offset;
    ctx->headland_env.start_point = env->start_point;
    ctx->headland_env.end_point = env->end_point;
    ctx->headland_env.boundary = ctx->headland.shrunken_zone;
    ctx->headland_env.obstacles = ctx->headland.expanded_obstacles;
    ctx->headland_env.obstacle_count = ctx->headland.expanded_obstacle_count;

    ctx->active_env = &ctx->headland_env;
    ctx->has_headland = true;

    printf("bounce_apply_headland: headland applied — shrunken zone, %u expanded obstacle(s)\n",
           ctx->headland.expanded_obstacle_count);

    return BOUNCE_STEP_OK;
}
