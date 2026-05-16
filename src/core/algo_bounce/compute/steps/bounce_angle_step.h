/**
 * bounce_angle_step.h
 *
 * Step 2: Selects the next travel angle for a bounce segment.
 *
 * On the first call (ctx->angle_initialized == false), an initial angle is
 * selected randomly within [0, 2pi). On subsequent calls (after a collision),
 * the angle is updated to reflect a valid reflection direction.
 *
 * A valid angle must:
 *   - Not point immediately toward the active environment boundary.
 *   - Travel into the interior of the active environment.
 *   - (Optional) apply a random offset within [-bounce_offset, +bounce_offset]
 *     radians after each reflection.
 *
 * Returns BOUNCE_STEP_OK  with ctx->current_angle set on success.
 * Returns BOUNCE_STEP_RETRY if the candidate angle is invalid (caller retries).
 * Returns BOUNCE_STEP_FAIL  if no valid angle can be found.
 */

#ifndef BOUNCE_ANGLE_STEP_H
#define BOUNCE_ANGLE_STEP_H

#include "../compute_runner/bounce_runner.h"

bounce_step_status_t bounce_pick_angle(bounce_pipeline_context_t *ctx);

#endif // BOUNCE_ANGLE_STEP_H
