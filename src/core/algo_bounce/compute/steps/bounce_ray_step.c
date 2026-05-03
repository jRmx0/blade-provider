/**
 * bounce_ray_step.c
 *
 * Step 3: Casts a ray from ctx->current_position in the direction of
 * ctx->current_angle and finds the nearest intersection with the boundary or
 * any obstacle of the active environment.
 *
 * On success:
 *   - segment_out is filled with the start/end waypoints.
 *   - ctx->hit_normal is set to the outward-facing normalized edge normal at
 *     the collision point, so bounce_pick_angle can compute the reflection on
 *     the next iteration.
 *   - ctx->has_hit_normal is set to true.
 *
 * Returns BOUNCE_STEP_RETRY if no intersection is found (starting position is
 * outside the active environment). Returns BOUNCE_STEP_FAIL on unrecoverable
 * error.
 *
 * Dependencies: bounce_ray_step.h
 */

#include "bounce_ray_step.h"

#include <math.h>
#include <float.h>

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

// Minimum t to avoid self-intersection with the edge the ray just left.
#define BOUNCE_RAY_TMIN 1e-4f
// Parallelism threshold for the cross-product denominator.
#define BOUNCE_RAY_EPSILON 1e-6f

/**
 * Compute the intersection of ray (origin + t * dir) with segment [A, B].
 *
 * Uses the parametric form:
 *   origin + t * dir = A + u * (B - A)
 *
 * Solved via 2D cross products:
 *   let e = B - A,  q = A - origin
 *   denom = cross(dir, e) = dir.x * e.y - dir.y * e.x
 *   t     = cross(q, e)   / denom
 *   u     = cross(q, dir) / denom
 *
 * Valid intersection: t >= BOUNCE_RAY_TMIN and u in [0, 1].
 *
 * Returns true on valid intersection and writes t_out, u_out.
 */
static bool ray_intersect_edge(
    point_t origin, point_t dir,
    point_t A, point_t B,
    float *t_out, float *u_out)
{
    float ex = B.x - A.x;
    float ey = B.y - A.y;

    float qx = A.x - origin.x;
    float qy = A.y - origin.y;

    float denom = dir.x * ey - dir.y * ex;
    if (fabsf(denom) < BOUNCE_RAY_EPSILON)
    {
        return false; // Ray is parallel to the edge.
    }

    float t = (qx * ey - qy * ex) / denom;
    float u = (qx * dir.y - qy * dir.x) / denom;

    if (t < BOUNCE_RAY_TMIN || u < 0.0f || u > 1.0f)
    {
        return false;
    }

    *t_out = t;
    *u_out = u;
    return true;
}

/**
 * Compute the normalized outward-facing normal for edge [A, B] relative to
 * the incoming ray direction. "Outward-facing" means dot(normal, dir) < 0
 * (the normal points toward the incoming ray, opposing it).
 */
static point_t edge_normal_facing_ray(point_t A, point_t B, point_t dir)
{
    float ex = B.x - A.x;
    float ey = B.y - A.y;

    // Perpendicular: rotate edge vector 90 degrees CCW → (-ey, ex)
    float nx = -ey;
    float ny = ex;

    float len = sqrtf(nx * nx + ny * ny);
    if (len > 1e-9f)
    {
        nx /= len;
        ny /= len;
    }

    // Ensure the normal opposes the incoming ray direction.
    if (nx * dir.x + ny * dir.y > 0.0f)
    {
        nx = -nx;
        ny = -ny;
    }

    return (point_t){nx, ny};
}

// ---------------------------------------------------------------------------
// Step implementation
// ---------------------------------------------------------------------------

bounce_step_status_t bounce_cast_ray(
    bounce_pipeline_context_t *ctx,
    bounce_segment_t *segment_out)
{
    if (ctx == NULL || segment_out == NULL || ctx->active_env == NULL)
    {
        return BOUNCE_STEP_FAIL;
    }

    point_t origin = ctx->current_position;
    point_t dir = {
        cosf(ctx->current_angle),
        sinf(ctx->current_angle)};

    const input_environment_t *env = ctx->active_env;

    float best_t = FLT_MAX;
    point_t best_normal = {0.0f, 0.0f};

    // --- Test boundary edges ---
    for (uint32_t i = 0; env->boundary.edges != NULL && i < env->boundary.edge_count; ++i)
    {
        point_t A = env->boundary.edges[i].begin;
        point_t B = env->boundary.edges[i].end;
        float t, u;
        if (ray_intersect_edge(origin, dir, A, B, &t, &u) && t < best_t)
        {
            best_t = t;
            best_normal = edge_normal_facing_ray(A, B, dir);
        }
    }

    // --- Test obstacle edges ---
    for (uint32_t k = 0; env->obstacles != NULL && k < env->obstacle_count; ++k)
    {
        const polygon_t *obs = &env->obstacles[k];
        for (uint32_t i = 0; obs->edges != NULL && i < obs->edge_count; ++i)
        {
            point_t A = obs->edges[i].begin;
            point_t B = obs->edges[i].end;
            float t, u;
            if (ray_intersect_edge(origin, dir, A, B, &t, &u) && t < best_t)
            {
                best_t = t;
                best_normal = edge_normal_facing_ray(A, B, dir);
            }
        }
    }

    if (best_t >= FLT_MAX)
    {
        // No intersection: starting position is outside the active environment.
        return BOUNCE_STEP_RETRY;
    }

    segment_out->start = origin;
    segment_out->end.x = origin.x + best_t * dir.x;
    segment_out->end.y = origin.y + best_t * dir.y;

    // Store collision normal so bounce_pick_angle can reflect on the next iteration.
    ctx->hit_normal = best_normal;
    ctx->has_hit_normal = true;

    return BOUNCE_STEP_OK;
}
