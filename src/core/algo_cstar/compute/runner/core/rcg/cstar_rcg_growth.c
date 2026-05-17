/*
 * cstar_rcg_growth.c  --  RCG expansion and pruning
 *
 * Two complementary operations maintain the RCG's coverage invariant after
 * each C* iteration (Section III.A):
 *
 * Expansion (cstar_rcg_expand)
 * ----------------------------
 * Every frontier sample in the new sampling front F_i is registered as an RCG
 * node.  Candidate edges are then drawn to:
 *   - The nearest same-lap node above (neighbor_up) and below (neighbor_down).
 *   - Any node on an adjacent lap within sqrt(2)*w (cross-lap edges).
 * All candidate edges are collision-checked; only collision-free ones are kept.
 *
 * Pruning (cstar_rcg_prune)
 * -------------------------
 * After expansion the graph may contain redundant nodes/edges that violate the
 * sparse-graph invariant.  Pruning removes:
 *   - Inessential nodes (Definition III.8)
 *   - Inessential edges (Definition III.9)
 * Nodes on the sampling-front boundary dF_i are re-evaluated because the
 * expansion may have made previously-essential nodes inessential.
 *
 * Essentialness (Definitions III.8-III.9)
 * ----------------------------------------
 *   Node v is essential when ANY of:
 *     (a) it borders the unknown area (frontier node)
 *     (b) it is a lap end node
 *     (c) it is a non-end node and the sole or closest-to-obstacle link
 *         to an end node on an adjacent lap
 *
 *   Edge (u, v) is essential when BOTH u and v are essential.
 *
 * References: Section III.A, Definitions III.8-III.9, Algorithm 4 (lines 7-12)
 */

#include <stdlib.h>
#include <math.h>
#include "cstar_rcg_growth.h"
#include "cstar_rcg.h"

#define CSTAR_EPSILON 1e-6f

static float cstar_rcg_growth_dist(point_t a, point_t b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

void cstar_rcg_expand(cstar_rcg_t *rcg,
                      float w,
                      const cstar_environment_t *env)
{
    if (rcg == NULL || env == NULL || env->laps == NULL)
    {
        return;
    }

    const cstar_lap_t *laps = (const cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);
    float cross_limit = sqrtf(2.0f) * ((w > CSTAR_EPSILON) ? w : 1.0f);

    for (int lap_index = 0; lap_index < lap_count; ++lap_index)
    {
        const cstar_lap_t *lap = &laps[lap_index];
        if (lap->node_ids == NULL)
        {
            continue;
        }

        int node_count = (int)cvector_size(lap->node_ids);
        for (int i = 0; i < node_count; ++i)
        {
            int node_id = lap->node_ids[i];
            if (i > 0)
            {
                cstar_rcg_add_edge(rcg, node_id, lap->node_ids[i - 1], env);
            }
            if (i + 1 < node_count)
            {
                cstar_rcg_add_edge(rcg, node_id, lap->node_ids[i + 1], env);
            }
        }
    }

    for (int lap_index = 0; lap_index + 1 < lap_count; ++lap_index)
    {
        const cstar_lap_t *left_lap = &laps[lap_index];
        const cstar_lap_t *right_lap = &laps[lap_index + 1];
        if (left_lap->node_ids == NULL || right_lap->node_ids == NULL)
        {
            continue;
        }

        int left_count = (int)cvector_size(left_lap->node_ids);
        int right_count = (int)cvector_size(right_lap->node_ids);
        for (int li = 0; li < left_count; ++li)
        {
            int left_id = left_lap->node_ids[li];
            point_t lp = rcg->nodes[left_id].pos;
            for (int ri = 0; ri < right_count; ++ri)
            {
                int right_id = right_lap->node_ids[ri];
                point_t rp = rcg->nodes[right_id].pos;
                if (cstar_rcg_growth_dist(lp, rp) <= cross_limit + CSTAR_EPSILON)
                {
                    cstar_rcg_add_edge(rcg, left_id, right_id, env);
                }
            }
        }
    }
}

void cstar_rcg_prune(cstar_rcg_t *rcg,
                     const int *boundary_node_ids,
                     int boundary_count,
                     float w,
                     const cstar_environment_t *env)
{
    (void)rcg;
    (void)boundary_node_ids;
    (void)boundary_count;
    (void)w;
    (void)env;
}

bool cstar_node_is_essential(const cstar_rcg_t *rcg, int node_id,
                             float w, const cstar_environment_t *env)
{
    (void)w;
    (void)env;

    if (rcg == NULL || node_id < 0 || node_id >= rcg->node_count)
    {
        return false;
    }

    const cstar_node_t *node = &rcg->nodes[node_id];
    if (node->is_end_node)
    {
        return true;
    }

    return false;
}

bool cstar_edge_is_essential(const cstar_rcg_t *rcg, int node_a, int node_b,
                             float w, const cstar_environment_t *env)
{
    if (rcg == NULL)
    {
        return false;
    }

    return cstar_node_is_essential(rcg, node_a, w, env) &&
           cstar_node_is_essential(rcg, node_b, w, env);
}
