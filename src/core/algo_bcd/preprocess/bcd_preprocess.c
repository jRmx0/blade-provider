/**
 * bcd_preprocess.c
 *
 * Implementation of the BCD input environment preprocessor.
 * See bcd_preprocess.h for the public interface.
 */

#include <math.h>
#include <stdlib.h>
#include "bcd_preprocess.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Two vertex projections are considered colliding when they are closer than
 * this threshold along the sweep axis. */
#define COLLISION_EPSILON 1e-4f

/* Each colliding vertex (after the first in a group) is nudged by a multiple
 * of this step along the sweep axis: rank * COLLISION_NUDGE. */
#define COLLISION_NUDGE 1e-4f

// TYPES -----------------------------------------------------------

typedef struct
{
    polygon_t *polygon;
    int        vertex_index;
    float      projection;
} vertex_proj_entry_t;

// FORWARD DECLARATIONS --------------------------------------------

static int  collect_projections(input_environment_t *env,
                                float ax, float ay,
                                vertex_proj_entry_t *entries);

static int  compare_proj_entry(const void *a, const void *b);

static void resolve_collision_group(vertex_proj_entry_t *entries,
                                    int group_start, int group_end,
                                    float ax, float ay);

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
    float ax  = cosf(rad);
    float ay  = sinf(rad);

    /* Count total vertices across boundary and all obstacles. */
    int total = (int)env->boundary.vertex_count;
    for (uint32_t i = 0; i < env->obstacle_count; i++)
        total += (int)env->obstacles[i].vertex_count;

    if (total < 2)
        return 0; /* nothing to collide */

    /* Allocate flat projection array. */
    vertex_proj_entry_t *entries =
        (vertex_proj_entry_t *)malloc((size_t)total * sizeof(vertex_proj_entry_t));
    if (!entries)
        return -4;

    /* Fill projection values. */
    collect_projections(env, ax, ay, entries);

    /* Sort by projection along sweep axis. */
    qsort(entries, (size_t)total, sizeof(vertex_proj_entry_t), compare_proj_entry);

    /* Walk sorted array and resolve collision groups. */
    int i = 0;
    while (i < total)
    {
        /* Extend the group as long as projections are within COLLISION_EPSILON
         * of the group's first entry. */
        int j = i + 1;
        while (j < total &&
               (entries[j].projection - entries[i].projection) < COLLISION_EPSILON)
        {
            j++;
        }

        if (j - i >= 2)
            resolve_collision_group(entries, i, j, ax, ay);

        i = j;
    }

    free(entries);
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
        point_t *pt          = &env->boundary.vertices[v];
        entries[idx].polygon      = &env->boundary;
        entries[idx].vertex_index = v;
        entries[idx].projection   = pt->x * ax + pt->y * ay;
        idx++;
    }

    /* Obstacle vertices. */
    for (uint32_t i = 0; i < env->obstacle_count; i++)
    {
        polygon_t *obs = &env->obstacles[i];
        for (int v = 0; v < (int)obs->vertex_count; v++)
        {
            point_t *pt          = &obs->vertices[v];
            entries[idx].polygon      = obs;
            entries[idx].vertex_index = v;
            entries[idx].projection   = pt->x * ax + pt->y * ay;
            idx++;
        }
    }

    return idx;
}

static int compare_proj_entry(const void *a, const void *b)
{
    const vertex_proj_entry_t *va = (const vertex_proj_entry_t *)a;
    const vertex_proj_entry_t *vb = (const vertex_proj_entry_t *)b;
    if (va->projection < vb->projection) return -1;
    if (va->projection > vb->projection) return  1;
    return 0;
}

static void resolve_collision_group(vertex_proj_entry_t *entries,
                                    int group_start, int group_end,
                                    float ax, float ay)
{
    /* The first entry in the group keeps its position.
     * Each subsequent entry is nudged by rank * COLLISION_NUDGE along the
     * sweep axis: 1*nudge, 2*nudge, 3*nudge, … */
    for (int k = group_start + 1; k < group_end; k++)
    {
        int   rank  = k - group_start; /* 1, 2, 3, … */
        float nudge = (float)rank * COLLISION_NUDGE;

        polygon_t *poly = entries[k].polygon;
        int        vi   = entries[k].vertex_index;

        poly->vertices[vi].x += nudge * ax;
        poly->vertices[vi].y += nudge * ay;

        recompute_adjacent_edges(poly, vi);
    }
}

static void recompute_adjacent_edges(polygon_t *poly, int vi)
{
    int N = (int)poly->vertex_count;

    /* Edge[vi]         : begins at vertex[vi], ends at vertex[(vi+1)%N] */
    poly->edges[vi].begin = poly->vertices[vi];

    /* Edge[(vi-1+N)%N] : begins at vertex[(vi-1+N)%N], ends at vertex[vi] */
    poly->edges[(vi - 1 + N) % N].end = poly->vertices[vi];
}
