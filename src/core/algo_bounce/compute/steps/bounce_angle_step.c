/**
 * bounce_angle_step.c
 *
 * Step 2: Selects a random valid travel angle for the next bounce segment.
 *
 * TODO: Implement proper angle-validity checks and post-collision reflection.
 *       Valid angle rules:
 *         1. A ray from current_position at theta must not immediately exit
 *            the active_env boundary (point must be inside polygon).
 *         2. After a collision, reflect the incoming angle off the hit edge
 *            (angle of incidence == angle of reflection), then apply a random
 *            offset within [-bounce_offset, +bounce_offset] radians from
 *            active_env->bounce_offset.
 *
 * Current stub selects a uniform random angle in [0, 2pi) without validation.
 *
 * Dependencies: bounce_angle_step.h
 */

#include "bounce_angle_step.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bounce_step_status_t bounce_pick_angle(bounce_pipeline_context_t *ctx)
{
    // TODO: Replace stub with validated angle selection.
    //
    // First iteration: pick a random starting angle in [0, 2pi).
    // Subsequent iterations: compute reflected angle from the collision edge
    // normal, then apply bounce_offset noise.
    //
    // Validity check (to implement):
    //   - Cast a short test ray at the candidate angle from current_position.
    //   - If it immediately exits active_env->boundary, return BOUNCE_STEP_RETRY.

    float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * (float)M_PI;

    ctx->current_angle = angle;
    ctx->angle_initialized = true;

    printf("bounce_pick_angle: iter %d — angle=%.4f rad (%.2f deg)\n",
           ctx->metrics.iteration,
           angle,
           angle * 180.0f / (float)M_PI);

    return BOUNCE_STEP_OK;
}
