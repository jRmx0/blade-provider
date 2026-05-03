/**
 * bounce_angle_step.c
 *
 * Step 2: Selects the travel angle for the next bounce segment.
 *
 * First iteration (has_hit_normal == false):
 *   Picks a uniform random angle in [0, 2π).
 *
 * Subsequent iterations (has_hit_normal == true):
 *   1. Reflects the incoming direction off ctx->hit_normal using the standard
 *      specular reflection formula: r = d - 2*(d·n)*n.
 *   2. Applies a random angular perturbation in
 *      [-max_offset, +max_offset] where max_offset is derived from
 *      active_env->bounce_offset (0–100 mapped to 0–π/2 radians).
 *
 * The resulting angle is always valid by construction: reflection off the hit
 * edge always points away from the boundary into the interior.
 *
 * Dependencies: bounce_angle_step.h
 */

#include "bounce_angle_step.h"

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bounce_step_status_t bounce_pick_angle(bounce_pipeline_context_t *ctx)
{
    float angle;

    if (!ctx->has_hit_normal)
    {
        // First iteration: pick a fully random starting direction in [0, 2π).
        angle = ((float)rand() / (float)RAND_MAX) * 2.0f * (float)M_PI;
    }
    else
    {
        // Compute the reflected direction off the last collision edge normal.
        float dx = cosf(ctx->current_angle);
        float dy = sinf(ctx->current_angle);
        float nx = ctx->hit_normal.x;
        float ny = ctx->hit_normal.y;

        // Specular reflection: r = d - 2*(d·n)*n
        float dot = dx * nx + dy * ny;
        float rx = dx - 2.0f * dot * nx;
        float ry = dy - 2.0f * dot * ny;

        float base_angle = atan2f(ry, rx);

        // Apply a random perturbation bounded by the bounce offset.
        // bounce_offset is in [0, 100]; map to [0, π/2] radians max deviation.
        float max_offset_rad = (ctx->active_env->bounce_offset / 100.0f) *
                               ((float)M_PI / 2.0f);
        float noise = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) *
                      max_offset_rad;

        angle = base_angle + noise;
    }

    ctx->current_angle = angle;
    ctx->angle_initialized = true;

    return BOUNCE_STEP_OK;
}
