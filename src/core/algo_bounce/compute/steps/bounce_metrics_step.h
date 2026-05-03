/**
 * bounce_metrics_step.h
 *
 * Step 4: Updates cumulative path metrics after a segment is committed.
 *
 * - Increments ctx->metrics.total_distance by the Euclidean segment length.
 * - Updates ctx->metrics.estimated_coverage based on the swept area vs.
 *   total active environment area.
 *
 * TODO: Implement polygon-clipped area estimation for accurate coverage.
 *       Current stub accumulates distance only; coverage stays at 0 until
 *       real geometry is wired.
 */

#ifndef BOUNCE_METRICS_STEP_H
#define BOUNCE_METRICS_STEP_H

#include "../compute_runner/bounce_runner.h"

void bounce_update_metrics(
    bounce_pipeline_context_t *ctx,
    const bounce_segment_t *segment);

#endif // BOUNCE_METRICS_STEP_H
