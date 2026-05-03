/**
 * bounce_headland_step.h
 *
 * Step 1: Applies optional headland conversion to the pipeline context.
 *
 * When environment->headland is true:
 *   - Runs compute_headland() against the original environment.
 *   - Stores headland data in ctx->headland.
 *   - Builds a shallow headland_env wrapper pointing at shrunken zone and
 *     expanded obstacles, mirroring the BCD runner pattern.
 *   - Sets ctx->active_env = &ctx->headland_env.
 *   - Sets ctx->has_headland = true.
 *
 * When environment->headland is false:
 *   - Sets ctx->active_env = env.
 *   - Sets ctx->has_headland = false.
 *
 * Returns BOUNCE_STEP_OK on success, BOUNCE_STEP_FAIL on error.
 */

#ifndef BOUNCE_HEADLAND_STEP_H
#define BOUNCE_HEADLAND_STEP_H

#include "../compute_runner/bounce_runner.h"

bounce_step_status_t bounce_apply_headland(
    input_environment_t *env,
    bounce_pipeline_context_t *ctx);

#endif // BOUNCE_HEADLAND_STEP_H
