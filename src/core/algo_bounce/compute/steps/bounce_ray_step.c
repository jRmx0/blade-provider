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

// Minimum t to reject genuinely degenerate intersections (opposite-direction
// hits, near-zero-length segments from numeric noise unrelated to the last
// edge).  The last hit edge is now excluded by exact endpoint comparison so
// TMIN does not need to cover the float-lerp residual (~0.06 m for Mercator).
#define BOUNCE_RAY_TMIN 1e-6
// Parallelism threshold for the double-precision cross-product denominator.
#define BOUNCE_RAY_EPSILON 1e-12
// u-parameter threshold to classify a hit as a vertex hit.
#define BOUNCE_RAY_VERTEX_EPSILON 1e-4f

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
 * All intermediate arithmetic is done in double precision to avoid
 * catastrophic cancellation when coordinates are large (thousands of metres).
 * float32 subtraction of two nearly-equal large values loses all significant
 * digits in the cross-product terms, producing wildly wrong t/u values and
 * causing the nearest edge to be skipped in favour of a far-side one.
 *
 * Returns true on valid intersection and writes t_out, u_out.
 */
static bool ray_intersect_edge(
    point_t origin, point_t dir,
    point_t A, point_t B,
    float *t_out, float *u_out)
{
    double ex = (double)B.x - (double)A.x;
    double ey = (double)B.y - (double)A.y;

    double qx = (double)A.x - (double)origin.x;
    double qy = (double)A.y - (double)origin.y;

    double dx = (double)dir.x;
    double dy = (double)dir.y;

    double denom = dx * ey - dy * ex;
    if (denom < BOUNCE_RAY_EPSILON && denom > -BOUNCE_RAY_EPSILON)
    {
        return false; // Ray is parallel to the edge.
    }

    double t = (qx * ey - qy * ex) / denom;
    double u = (qx * dy - qy * dx) / denom;

    if (t < BOUNCE_RAY_TMIN || u < 0.0 || u > 1.0)
    {
        return false;
    }

    *t_out = (float)t;
    *u_out = (float)u;
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

/**
 * When a ray hits exactly at a polygon vertex, that vertex is shared by two
 * adjacent edges whose individual normals may differ significantly (especially
 * at sharp corners).  Using either edge's normal alone can produce a reflection
 * angle that exits through the boundary.  Instead, accumulate the facing normal
 * of every edge in the environment whose endpoint equals the hit vertex and
 * return their normalized average (the bisector normal).
 *
 * Edges are compared by exact float equality because they are built from the
 * same vertex array: edges[i].end == edges[(i+1)%n].begin exactly.
 */
static point_t blend_vertex_normal(
    const input_environment_t *env,
    point_t hit_vertex,
    point_t base_normal,
    point_t dir)
{
    float nx = base_normal.x;
    float ny = base_normal.y;

    // Helper: accumulate facing normal if this edge touches hit_vertex.
#define ACCUM_EDGE(A, B)                                                 \
    do                                                                   \
    {                                                                    \
        bool a_match = ((A).x == hit_vertex.x && (A).y == hit_vertex.y); \
        bool b_match = ((B).x == hit_vertex.x && (B).y == hit_vertex.y); \
        if (a_match || b_match)                                          \
        {                                                                \
            point_t n = edge_normal_facing_ray((A), (B), dir);           \
            nx += n.x;                                                   \
            ny += n.y;                                                   \
        }                                                                \
    } while (0)

    for (uint32_t i = 0; env->boundary.edges != NULL && i < env->boundary.edge_count; ++i)
    {
        point_t A = env->boundary.edges[i].begin;
        point_t B = env->boundary.edges[i].end;
        ACCUM_EDGE(A, B);
    }
    for (uint32_t k = 0; env->obstacles != NULL && k < env->obstacle_count; ++k)
    {
        const polygon_t *obs = &env->obstacles[k];
        for (uint32_t i = 0; obs->edges != NULL && i < obs->edge_count; ++i)
        {
            point_t A = obs->edges[i].begin;
            point_t B = obs->edges[i].end;
            ACCUM_EDGE(A, B);
        }
    }

#undef ACCUM_EDGE

    float len = sqrtf(nx * nx + ny * ny);
    if (len > 1e-9f)
    {
        nx /= len;
        ny /= len;
    }
    else
    {
        // Degenerate sum (opposing normals cancel); fall back to base normal.
        nx = base_normal.x;
        ny = base_normal.y;
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

    // No origin bias: current_position is snapped exactly to the previous hit
    // edge via lerp (best_A + best_u*(best_B-best_A)), so the self-intersection
    // is avoided by explicitly skipping the last hit edge (stored in ctx) rather
    // than relying on TMIN.  A forward nudge would push the origin outside the
    // polygon at any concave corner, causing the ray to find a far-side boundary
    // instead of the nearest one.

    const input_environment_t *env = ctx->active_env;

    // Precompute skip-edge endpoints for exact comparison.
    bool skip_edge = ctx->has_last_hit_edge;
    point_t skip_A = ctx->last_hit_edge_A;
    point_t skip_B = ctx->last_hit_edge_B;

#define IS_SKIP_EDGE(A, B)                     \
    (skip_edge &&                              \
     (A).x == skip_A.x && (A).y == skip_A.y && \
     (B).x == skip_B.x && (B).y == skip_B.y)

    float best_t = FLT_MAX;
    float best_u = 0.0f;
    point_t best_A = {0.0f, 0.0f};
    point_t best_B = {0.0f, 0.0f};
    point_t best_normal = {0.0f, 0.0f};

    // --- Test boundary edges ---
    for (uint32_t i = 0; env->boundary.edges != NULL && i < env->boundary.edge_count; ++i)
    {
        point_t A = env->boundary.edges[i].begin;
        point_t B = env->boundary.edges[i].end;
        if (IS_SKIP_EDGE(A, B))
        {
            continue;
        }
        float t, u;
        if (ray_intersect_edge(origin, dir, A, B, &t, &u) && t < best_t)
        {
            best_t = t;
            best_u = u;
            best_A = A;
            best_B = B;
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
            if (IS_SKIP_EDGE(A, B))
            {
                continue;
            }
            float t, u;
            if (ray_intersect_edge(origin, dir, A, B, &t, &u) && t < best_t)
            {
                best_t = t;
                best_u = u;
                best_A = A;
                best_B = B;
                best_normal = edge_normal_facing_ray(A, B, dir);
            }
        }
    }

#undef IS_SKIP_EDGE

    if (best_t >= FLT_MAX)
    {
        // No intersection: starting position is outside the active environment.
        return BOUNCE_STEP_RETRY;
    }

    segment_out->start = ctx->current_position;
    // Pin the endpoint to the exact edge location via linear interpolation of
    // the hit edge's vertices. Using origin + t*dir accumulates float32 error
    // that can drift current_position outside the polygon over many bounces,
    // causing subsequent ray casts to start from outside and cross the boundary.
    segment_out->end.x = best_A.x + best_u * (best_B.x - best_A.x);
    segment_out->end.y = best_A.y + best_u * (best_B.y - best_A.y);

    // Vertex hit: u ≈ 0 means the hit point is best_A; u ≈ 1 means best_B.
    // Two adjacent edges share that vertex, so a single edge normal is
    // ambiguous (and may point in the wrong half-plane after reflection).
    // Blend all edge normals that touch the vertex to get the bisector.
    bool is_vertex_hit = (best_u < BOUNCE_RAY_VERTEX_EPSILON ||
                          best_u > 1.0f - BOUNCE_RAY_VERTEX_EPSILON);
    if (is_vertex_hit)
    {
        best_normal = blend_vertex_normal(env, segment_out->end, best_normal, dir);
    }

    // Remember which edge was hit so the next ray cast can skip it exactly,
    // avoiding re-intersection due to float-lerp residual in current_position.
    ctx->last_hit_edge_A = best_A;
    ctx->last_hit_edge_B = best_B;
    ctx->has_last_hit_edge = true;

    // Store collision normal so bounce_pick_angle can reflect on the next iteration.
    ctx->hit_normal = best_normal;
    ctx->has_hit_normal = true;

    return BOUNCE_STEP_OK;
}
