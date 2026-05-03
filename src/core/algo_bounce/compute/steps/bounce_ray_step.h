/**
 * bounce_ray_step.h
 *
 * Step 3: Casts a straight ray from ctx->current_position in the direction of
 * ctx->current_angle until the ray intersects the boundary of the active
 * environment or an obstacle.
 *
 * On success, fills *segment_out with the start/end waypoints and returns
 * BOUNCE_STEP_OK. The caller is responsible for:
 *   - Appending segment_out to the coverage path.
 *   - Advancing ctx->current_position to segment_out->end.
 *   - Updating ctx->current_angle to the reflected direction (typically
 *     handled by bounce_pick_angle on the next iteration).
 *
 * Returns BOUNCE_STEP_RETRY if the starting position lies outside the active
 * environment (caller should re-pick the angle).
 * Returns BOUNCE_STEP_FAIL  on unrecoverable error.
 */

#ifndef BOUNCE_RAY_STEP_H
#define BOUNCE_RAY_STEP_H

#include "../compute_runner/bounce_runner.h"

bounce_step_status_t bounce_cast_ray(
    bounce_pipeline_context_t *ctx,
    bounce_segment_t *segment_out);

#endif // BOUNCE_RAY_STEP_H
