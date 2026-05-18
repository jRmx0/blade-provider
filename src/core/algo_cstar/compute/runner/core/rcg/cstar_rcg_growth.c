#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cstar_rcg_growth.h"
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

#define CSTAR_RCG_EPSILON 1e-6f

// -------------------------------------------------------------------------
// Graph Growth Helper Functions
// -------------------------------------------------------------------------

/**
 * Computes Euclidean distance between two nodes.
 */
static float cstar_rcg_node_distance(const cstar_node_t *a, const cstar_node_t *b)
{
    if (a == NULL || b == NULL)
    {
        return INFINITY;
    }

    float dx = a->pos.x - b->pos.x;
    float dy = a->pos.y - b->pos.y;
    return sqrtf(dx * dx + dy * dy);
}

/**
 * Adds an edge to the RCG.
 */
static void cstar_rcg_add_edge(cstar_rcg_t *rcg,
                               int node_a,
                               int node_b,
                               float cost)
{
    if (rcg == NULL)
    {
        return;
    }

    if (cstar_rcg_index_from_node_id(rcg, node_a) == CSTAR_NO_NEIGHBOR ||
        cstar_rcg_index_from_node_id(rcg, node_b) == CSTAR_NO_NEIGHBOR)
    {
        return;
    }

    cstar_edge_t edge = {0};
    edge.node_a = node_a;
    edge.node_b = node_b;
    edge.cost = cost;

    cvector_push_back(rcg->edges, edge);
    rcg->edge_count = (int)cvector_size(rcg->edges);
    rcg->edge_capacity = (int)cvector_capacity(rcg->edges);
}

// -------------------------------------------------------------------------
// RCG Expansion Implementation
// -------------------------------------------------------------------------

bool cstar_rcg_expand_graph(cstar_rcg_t *rcg,
                            const cstar_environment_t *env)
{
    if (rcg == NULL || env == NULL || env->laps == NULL)
    {
        return false;
    }

    cstar_lap_t *laps = (cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);

    if (lap_count <= 0 || rcg->node_count <= 0)
    {
        return rcg->node_count == 0;
    }

    float w = env->path_width;
    float cross_lap_threshold = sqrtf(2.0f) * w;

    // (Vertical lap edge creation removed)

    // -----------------------------------------------------------------------
    // Stage 2: Cross-lap horizontal connectivity
    // -----------------------------------------------------------------------
    // Connect all nodes within √2*w distance across adjacent laps.
    // (Rule ii: all nodes within distance √2w on each adjacent lap)
    // Each node can connect to multiple nodes on an adjacent lap.

    for (int lap_idx = 0; lap_idx < lap_count - 1; ++lap_idx)
    {
        cstar_lap_t *lap_left = &laps[lap_idx];
        cstar_lap_t *lap_right = &laps[lap_idx + 1];

        if (lap_left->node_ids == NULL || lap_right->node_ids == NULL)
        {
            continue;
        }

        // Connect ALL node pairs between adjacent laps within threshold
        for (int i = 0; i < lap_left->node_count; ++i)
        {
            int left_node_id = lap_left->node_ids[i];
            int left_node_idx = cstar_rcg_index_from_node_id(rcg, left_node_id);
            if (left_node_idx == CSTAR_NO_NEIGHBOR)
            {
                continue;
            }

            cstar_node_t *left_node = &rcg->nodes[left_node_idx];

            for (int j = 0; j < lap_right->node_count; ++j)
            {
                int right_node_id = lap_right->node_ids[j];
                int right_node_idx = cstar_rcg_index_from_node_id(rcg, right_node_id);
                if (right_node_idx == CSTAR_NO_NEIGHBOR)
                {
                    continue;
                }

                cstar_node_t *right_node = &rcg->nodes[right_node_idx];
                float distance = cstar_rcg_node_distance(left_node, right_node);

                // Connect if within cross-lap threshold
                if (distance <= cross_lap_threshold + CSTAR_RCG_EPSILON)
                {
                    // Re-fetch pointers: cstar_rcg_add_edge only grows rcg->edges,
                    // but re-fetch nodes defensively in case of future refactors.
                    left_node = &rcg->nodes[left_node_idx];
                    right_node = &rcg->nodes[right_node_idx];

                    if (left_node->neighbors_right_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                        left_node->neighbors_right[left_node->neighbors_right_count++] = right_node->id;
                    if (right_node->neighbors_left_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                        right_node->neighbors_left[right_node->neighbors_left_count++] = left_node->id;

                    cstar_rcg_add_edge(rcg, left_node->id, right_node->id, distance);
                }
            }
        }
    }

    return true;
}
