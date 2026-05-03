/**
 * bounce_ray_step.c
 *
 * Step 3: Casts a straight ray from ctx->current_position in the direction of
 * ctx->current_angle until it intersects the active environment boundary or
 * an obstacle edge.
 *
 * TODO: Implement proper ray-polygon edge intersection.
 *       Implementation plan:
 *         1. Iterate over all edges of active_env->boundary and
 *            active_env->obstacles.
 *         2. For each edge, compute the ray-segment intersection using the
 *            standard parametric form: P + t*D = A + s*(B-A), solve for t >= 0
 *            and s in [0, 1].
 *         3. Select the nearest valid intersection (smallest positive t).
 *         4. Set segment_out->end to the intersection point.
 *         5. Store the collision edge normal so bounce_pick_angle can compute
 *            the reflection angle on the next iteration.
 *
 * Current stub advances by a fixed step (BOUNCE_STUB_SEGMENT_LENGTH) in the
 * current direction, ignoring boundaries.
 *
 * Dependencies: bounce_ray_step.h
 */

#include "bounce_ray_step.h"

#include <math.h>
#include <stdio.h>

// TODO: Replace with an environment-derived value (e.g. bounding box diagonal).
#define BOUNCE_STUB_SEGMENT_LENGTH 50.0f

bounce_step_status_t bounce_cast_ray(
    bounce_pipeline_context_t *ctx,
    bounce_segment_t *segment_out)
{
    // TODO: Replace stub with ray-polygon-edge intersection.
    //
    // Return BOUNCE_STEP_RETRY when current_position is outside active_env
    // (e.g. after a headland conversion moves the effective boundary).

    segment_out->start = ctx->current_position;
    segment_out->end.x = ctx->current_position.x +
                         BOUNCE_STUB_SEGMENT_LENGTH * cosf(ctx->current_angle);
    segment_out->end.y = ctx->current_position.y +
                         BOUNCE_STUB_SEGMENT_LENGTH * sinf(ctx->current_angle);

    printf("bounce_cast_ray: (%.2f, %.2f) -> (%.2f, %.2f)\n",
           segment_out->start.x, segment_out->start.y,
           segment_out->end.x, segment_out->end.y);

    return BOUNCE_STEP_OK;
}
