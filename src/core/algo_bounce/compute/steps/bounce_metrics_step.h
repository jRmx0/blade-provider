/**
 * bounce_metrics_step.h
 *
 * Step 4: Updates cumulative path metrics after a segment is committed.
 *
 * - Increments ctx->metrics.total_distance by the Euclidean segment length.
 * - Updates ctx->metrics.estimated_coverage using a grid-based capsule method:
 *   An AOI cell grid is built lazily from the realworld geometry when present,
 *   otherwise from the transformed environment on the first call. For each
 *   segment, cells within (path_width / 2) of the segment line are marked
 *   covered (capsule footprint). Each cell is counted at most once.
 *   estimated_coverage = (covered_cells / valid_cells) * 100.
 *
 * Cell size = "Coverage Grid Cell Size" when > 0, otherwise path_width / 6,
 * auto-scaled to stay within a 1 M cell budget.
 */

#ifndef BOUNCE_METRICS_STEP_H
#define BOUNCE_METRICS_STEP_H

#include "../compute_runner/bounce_runner.h"

void bounce_update_metrics(
    bounce_pipeline_context_t *ctx,
    const bounce_segment_t *segment);

#endif // BOUNCE_METRICS_STEP_H
