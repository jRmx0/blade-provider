/**
 * bounce_metrics_step.c
 *
 * Step 4: Updates cumulative path metrics after a segment is committed.
 *
 * TODO: Implement polygon-clipped swept-area estimation for accurate coverage.
 *       Suggested approach:
 *         swept_area += segment_length * active_env->path_width  (strip area).
 *         env_area    = polygon area of active_env->boundary minus obstacle areas.
 *         estimated_coverage = (min(swept_area, env_area) / env_area) * 100.0f.
 *
 * Current stub increments total_distance by the Euclidean segment length and
 * leaves estimated_coverage unchanged (0), so only target_distance drives the
 * stop condition until coverage math is wired.
 *
 * Dependencies: bounce_metrics_step.h
 */

#include "bounce_metrics_step.h"

#include <math.h>

void bounce_update_metrics(
    bounce_pipeline_context_t *ctx,
    const bounce_segment_t *segment)
{
    float dx = segment->end.x - segment->start.x;
    float dy = segment->end.y - segment->start.y;
    float length = sqrtf(dx * dx + dy * dy);

    ctx->metrics.total_distance += length;

    // TODO: Replace with swept-strip area accumulation.
    // ctx->metrics.estimated_coverage = ...;
}
