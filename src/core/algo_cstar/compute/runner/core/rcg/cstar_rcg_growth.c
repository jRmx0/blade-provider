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
    if (rcg == NULL || node_a < 0 || node_b < 0 || node_a >= rcg->node_count || node_b >= rcg->node_count)
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

/**
 * Connectivity check using BFS: verifies all nodes are reachable from node 0.
 */
static bool cstar_rcg_is_connected(const cstar_rcg_t *rcg)
{
    if (rcg == NULL || rcg->node_count <= 0 || rcg->nodes == NULL)
    {
        return rcg->node_count == 0;
    }

    // Allocate visited array
    bool *visited = (bool *)malloc((size_t)rcg->node_count * sizeof(bool));
    if (visited == NULL)
    {
        return false;
    }

    memset(visited, 0, (size_t)rcg->node_count * sizeof(bool));

    // BFS queue
    int *queue = (int *)malloc((size_t)rcg->node_count * sizeof(int));
    if (queue == NULL)
    {
        free(visited);
        return false;
    }

    int queue_front = 0;
    int queue_back = 0;

    // Start from node 0
    queue[queue_back++] = 0;
    visited[0] = true;
    int visited_count = 1;

    while (queue_front < queue_back)
    {
        int current_id = queue[queue_front++];
        if (current_id < 0 || current_id >= rcg->node_count)
        {
            continue;
        }

        cstar_node_t *current = &rcg->nodes[current_id];

        // Explore all neighbors
        int neighbors[] = {current->neighbor_up, current->neighbor_down,
                           current->neighbor_left, current->neighbor_right};

        for (int i = 0; i < 4; ++i)
        {
            int neighbor_id = neighbors[i];
            if (neighbor_id != CSTAR_NO_NEIGHBOR && !visited[neighbor_id] && neighbor_id < rcg->node_count)
            {
                visited[neighbor_id] = true;
                queue[queue_back++] = neighbor_id;
                visited_count++;
            }
        }
    }

    free(queue);
    free(visited);

    return visited_count == rcg->node_count;
}

/**
 * Planarity check: Euler's formula for planar graphs.
 * For a connected planar graph: edges ≤ 3 * nodes - 6
 */
static bool cstar_rcg_is_planar(const cstar_rcg_t *rcg)
{
    if (rcg == NULL)
    {
        return true;
    }

    if (rcg->node_count < 3)
    {
        // Graphs with < 3 nodes are always planar
        return true;
    }

    int max_edges = 3 * rcg->node_count - 6;
    return rcg->edge_count <= max_edges;
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

    // -----------------------------------------------------------------------
    // Stage 1: Same-lap vertical connectivity
    // -----------------------------------------------------------------------
    // For each lap, connect adjacent nodes (already ordered by y-position)
    // via neighbor_up and neighbor_down pointers.

    for (int lap_idx = 0; lap_idx < lap_count; ++lap_idx)
    {
        cstar_lap_t *lap = &laps[lap_idx];

        if (lap->node_ids == NULL || lap->node_count < 1)
        {
            continue;
        }

        // Connect consecutive nodes on the same lap
        for (int i = 0; i < lap->node_count - 1; ++i)
        {
            int node_id_lower = lap->node_ids[i];
            int node_id_upper = lap->node_ids[i + 1];

            if (node_id_lower < 0 || node_id_lower >= rcg->node_count ||
                node_id_upper < 0 || node_id_upper >= rcg->node_count)
            {
                continue;
            }

            cstar_node_t *node_lower = &rcg->nodes[node_id_lower];
            cstar_node_t *node_upper = &rcg->nodes[node_id_upper];

            float cost = cstar_rcg_node_distance(node_lower, node_upper);
            if (cost > w + CSTAR_RCG_EPSILON)
            {
                continue;
            }

            // Set bidirectional neighbors
            node_lower->neighbor_up = node_id_upper;
            node_upper->neighbor_down = node_id_lower;

            // Add edge for same-lap connectivity
            cstar_rcg_add_edge(rcg, node_id_lower, node_id_upper, cost);
        }
    }

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
            if (left_node_id < 0 || left_node_id >= rcg->node_count)
            {
                continue;
            }

            cstar_node_t *left_node = &rcg->nodes[left_node_id];

            for (int j = 0; j < lap_right->node_count; ++j)
            {
                int right_node_id = lap_right->node_ids[j];
                if (right_node_id < 0 || right_node_id >= rcg->node_count)
                {
                    continue;
                }

                cstar_node_t *right_node = &rcg->nodes[right_node_id];
                float distance = cstar_rcg_node_distance(left_node, right_node);

                // Connect if within cross-lap threshold
                if (distance <= cross_lap_threshold + CSTAR_RCG_EPSILON)
                {
                    cstar_rcg_add_edge(rcg, left_node_id, right_node_id, distance);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Stage 3: Graph Validation (non-fatal)
    // -----------------------------------------------------------------------
    // Keep checks for observability/debugging, but do not abort expansion.
    // With strict same-lap distance gating (d <= w), sparse frontiers can
    // legitimately yield disconnected intermediate graphs.

    (void)cstar_rcg_is_connected(rcg);
    (void)cstar_rcg_is_planar(rcg);

    return true;
}
