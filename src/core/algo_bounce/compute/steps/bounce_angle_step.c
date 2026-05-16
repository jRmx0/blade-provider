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

#define BOUNCE_ANGLE_MIN_OUTWARD_DOT 0.05f
#define BOUNCE_ANGLE_EPSILON 1e-6f

typedef struct
{
    float lo;
    float hi;
} bounce_angle_interval_t;

static float bounce_random_unit(void)
{
    return (float)rand() / (float)RAND_MAX;
}

static float bounce_clampf(float value, float min_v, float max_v)
{
    if (value < min_v)
        return min_v;
    if (value > max_v)
        return max_v;
    return value;
}

static float bounce_normalize_angle(float angle)
{
    const float two_pi = 2.0f * (float)M_PI;
    while (angle < 0.0f)
    {
        angle += two_pi;
    }
    while (angle >= two_pi)
    {
        angle -= two_pi;
    }
    return angle;
}

static int bounce_split_wrapped_interval(
    float start,
    float end,
    bounce_angle_interval_t out[2])
{
    const float two_pi = 2.0f * (float)M_PI;

    start = bounce_normalize_angle(start);
    end = bounce_normalize_angle(end);

    if (start <= end)
    {
        out[0].lo = start;
        out[0].hi = end;
        return 1;
    }

    out[0].lo = 0.0f;
    out[0].hi = end;
    out[1].lo = start;
    out[1].hi = two_pi;
    return 2;
}

static int bounce_intersect_interval_sets(
    const bounce_angle_interval_t *a,
    int a_count,
    const bounce_angle_interval_t *b,
    int b_count,
    bounce_angle_interval_t out[4])
{
    int count = 0;

    for (int i = 0; i < a_count; ++i)
    {
        for (int j = 0; j < b_count; ++j)
        {
            float lo = (a[i].lo > b[j].lo) ? a[i].lo : b[j].lo;
            float hi = (a[i].hi < b[j].hi) ? a[i].hi : b[j].hi;

            if (hi - lo > BOUNCE_ANGLE_EPSILON)
            {
                out[count].lo = lo;
                out[count].hi = hi;
                ++count;
            }
        }
    }

    return count;
}

static float bounce_sample_from_intervals(
    const bounce_angle_interval_t *intervals,
    int count,
    float fallback_angle)
{
    float total_len = 0.0f;
    for (int i = 0; i < count; ++i)
    {
        total_len += intervals[i].hi - intervals[i].lo;
    }

    if (total_len <= BOUNCE_ANGLE_EPSILON)
    {
        return fallback_angle;
    }

    float r = bounce_random_unit() * total_len;
    for (int i = 0; i < count; ++i)
    {
        float len = intervals[i].hi - intervals[i].lo;
        if (r <= len)
        {
            return intervals[i].lo + r;
        }
        r -= len;
    }

    // Numeric fallback (should be unreachable).
    return intervals[count - 1].hi;
}

bounce_step_status_t bounce_pick_angle(bounce_pipeline_context_t *ctx)
{
    if (ctx == NULL || ctx->active_env == NULL)
    {
        return BOUNCE_STEP_FAIL;
    }

    float angle;

    if (!ctx->has_hit_normal)
    {
        // First iteration: use the specified starting angle, or pick randomly.
        if (ctx->original_env->starting_angle >= 0.0f)
        {
            // Convert degrees [0, 360] → radians.
            angle = ctx->original_env->starting_angle * ((float)M_PI / 180.0f);
        }
        else
        {
            angle = bounce_random_unit() * 2.0f * (float)M_PI;
        }
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

        // Random-offset window around reflection.
        // bounce_offset is in [0, 100]; map to [0, π/2] radians max deviation.
        float max_offset_rad = (ctx->active_env->bounce_offset / 100.0f) *
                               ((float)M_PI / 2.0f);

        bounce_angle_interval_t offset_intervals[2];
        int offset_count = bounce_split_wrapped_interval(
            base_angle - max_offset_rad,
            base_angle + max_offset_rad,
            offset_intervals);

        // Outward-valid window: cos(theta - normal_angle) > MIN_DOT
        // => theta in [normal_angle - alpha, normal_angle + alpha] on circle,
        // where alpha = acos(MIN_DOT).
        float min_dot = bounce_clampf(BOUNCE_ANGLE_MIN_OUTWARD_DOT, -1.0f, 1.0f);
        float alpha = acosf(min_dot);
        float normal_angle = atan2f(ny, nx);

        bounce_angle_interval_t outward_intervals[2];
        int outward_count = 0;

        if (alpha >= (float)M_PI - BOUNCE_ANGLE_EPSILON)
        {
            outward_intervals[0].lo = 0.0f;
            outward_intervals[0].hi = 2.0f * (float)M_PI;
            outward_count = 1;
        }
        else
        {
            outward_count = bounce_split_wrapped_interval(
                normal_angle - alpha,
                normal_angle + alpha,
                outward_intervals);
        }

        bounce_angle_interval_t valid_intervals[4];
        int valid_count = bounce_intersect_interval_sets(
            offset_intervals,
            offset_count,
            outward_intervals,
            outward_count,
            valid_intervals);

        // Sample directly from valid angular ranges (range-first sampling).
        angle = bounce_sample_from_intervals(valid_intervals, valid_count, base_angle);
    }

    ctx->current_angle = bounce_normalize_angle(angle);
    ctx->angle_initialized = true;

    return BOUNCE_STEP_OK;
}
