/**
 * bcd_preprocess.c
 *
 * Implementation of the BCD input environment preprocessor.
 * See bcd_preprocess.h for the public interface.
 */

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "bcd_preprocess.h"
#include "../../../../../../../dependencies/allocator/allocator.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Two vertex projections are considered colliding when they are closer than
 * this threshold along the sweep axis. */
#define COLLISION_EPSILON 1e-4f

/* Guaranteed minimum separation placed between consecutive projections after
 * resolution.  Using 2× COLLISION_EPSILON ensures the post-nudge gap safely
 * exceeds the detection threshold even after float32 rounding. */
#define COLLISION_MIN_SEP 0.01

// TYPES -----------------------------------------------------------

typedef struct
{
    polygon_t *polygon;
    int vertex_index;
    float projection;
} vertex_proj_entry_t;

// FORWARD DECLARATIONS --------------------------------------------

static int collect_projections(input_environment_t *env,
                               float ax, float ay,
                               vertex_proj_entry_t *entries);

static int compare_proj_entry(const void *a, const void *b);

static void recompute_adjacent_edges(polygon_t *poly, int vi);

// IMPLEMENTATION --------------------------------------------------

int bcd_preprocess_environment(input_environment_t *env, float sweep_direction_deg)
{
    if (!env)
        return -1;

    /* Sweep axis unit vector.
     * sweep_direction_deg = 0  → axis = (1, 0)  → projection = raw x.
     * This matches the current BCD sweep behaviour exactly. */
    float rad = sweep_direction_deg * (float)(M_PI / 180.0);
    float ax = cosf(rad);
    float ay = sinf(rad);

    /* Count total vertices across boundary and all obstacles. */
    int total = (int)env->boundary.vertex_count;
    for (uint32_t i = 0; i < env->obstacle_count; i++)
        total += (int)env->obstacles[i].vertex_count;

    if (total < 2)
        return 0; /* nothing to collide */

    /* Allocate flat projection array. */
    vertex_proj_entry_t *entries =
        (vertex_proj_entry_t *)va_malloc((size_t)total * sizeof(vertex_proj_entry_t));
    if (!entries)
        return -4;

    /* Fill projection values. */
    collect_projections(env, ax, ay, entries);

    /* Sort by projection along sweep axis. */
    qsort(entries, (size_t)total, sizeof(vertex_proj_entry_t), compare_proj_entry);

    /* Forward scan: walk sorted entries left-to-right and push any entry whose
     * projection is too close to the previous one forward by the minimum amount.
     *
     * This single O(n) pass resolves arbitrarily long collision chains because:
     *   - entries are processed in ascending order,
     *   - entries[i].projection is updated in-place before the (i+1) comparison,
     *   - so a pushed vertex feeds the correct baseline into the next step.
     *
     * COLLISION_MIN_SEP = 2 × COLLISION_EPSILON ensures the resulting gap is
     * safely above the detection threshold even after float32 rounding. */
    for (int i = 1; i < total; i++)
    {
        float prev = entries[i - 1].projection;
        float curr = entries[i].projection;

        if (curr - prev < COLLISION_EPSILON)
        {
            /* COLLISION_MIN_SEP is sufficient for small coordinates, but at
             * large magnitudes (e.g. x ≈ 1e5 m) the float32 ULP exceeds it
             * and the nudge gets rounded away — leaving the vertex X unchanged.
             * Use at least 16 ULPs at the relevant magnitude so the nudge
             * always produces a representable float32 difference. */
            float mag = fabsf(prev) > fabsf(curr) ? fabsf(prev) : fabsf(curr);
            if (mag < 1.0f)
                mag = 1.0f;
            float dynamic_sep = mag * 16.0f * FLT_EPSILON;
            float sep = dynamic_sep > COLLISION_MIN_SEP ? dynamic_sep : COLLISION_MIN_SEP;

            float target = prev + sep;

            /* Paranoia: if float32 arithmetic rounded target back to prev,
             * advance one ULP at a time until it is strictly greater. */
            while (target <= prev)
                target = nextafterf(prev, prev + 1.0f);

            float delta = target - curr;

            polygon_t *poly = entries[i].polygon;
            int vi = entries[i].vertex_index;

            poly->vertices[vi].x += delta * ax;
            poly->vertices[vi].y += delta * ay;
            entries[i].projection = target; /* propagate for next iteration */

            recompute_adjacent_edges(poly, vi);
        }
    }

    va_free(entries);
    return 0;
}

// STATIC HELPERS --------------------------------------------------

static int collect_projections(input_environment_t *env,
                               float ax, float ay,
                               vertex_proj_entry_t *entries)
{
    int idx = 0;

    /* Boundary vertices. */
    for (int v = 0; v < (int)env->boundary.vertex_count; v++)
    {
        point_t *pt = &env->boundary.vertices[v];
        entries[idx].polygon = &env->boundary;
        entries[idx].vertex_index = v;
        entries[idx].projection = pt->x * ax + pt->y * ay;
        idx++;
    }

    /* Obstacle vertices. */
    for (uint32_t i = 0; i < env->obstacle_count; i++)
    {
        polygon_t *obs = &env->obstacles[i];
        for (int v = 0; v < (int)obs->vertex_count; v++)
        {
            point_t *pt = &obs->vertices[v];
            entries[idx].polygon = obs;
            entries[idx].vertex_index = v;
            entries[idx].projection = pt->x * ax + pt->y * ay;
            idx++;
        }
    }

    return idx;
}

static int compare_proj_entry(const void *a, const void *b)
{
    const vertex_proj_entry_t *va = (const vertex_proj_entry_t *)a;
    const vertex_proj_entry_t *vb = (const vertex_proj_entry_t *)b;
    if (va->projection < vb->projection)
        return -1;
    if (va->projection > vb->projection)
        return 1;
    return 0;
}

static void recompute_adjacent_edges(polygon_t *poly, int vi)
{
    int N = (int)poly->vertex_count;

    /* Edge[vi]         : begins at vertex[vi], ends at vertex[(vi+1)%N] */
    poly->edges[vi].begin = poly->vertices[vi];

    /* Edge[(vi-1+N)%N] : begins at vertex[(vi-1+N)%N], ends at vertex[vi] */
    poly->edges[(vi - 1 + N) % N].end = poly->vertices[vi];
}
