#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cstar_rcg_growth.h"
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"
#include "../../../../../common/clog.h"

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
void cstar_rcg_add_edge(cstar_rcg_t *rcg,
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

    // -----------------------------------------------------------------------
    // Cross-lap horizontal connectivity
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

                    // Don't expose start_point as a cross-lap neighbour of
                    // other nodes (see cstar_rcg_rebuild_links_from_edges for
                    // rationale). start_point's OWN arrays are still populated
                    // so the robot can navigate away on the first iteration.
                    // The edge is always recorded for A* dead-end escape.
                    if (!right_node->is_start_point &&
                        left_node->neighbors_right_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                        left_node->neighbors_right[left_node->neighbors_right_count++] = right_node->id;
                    if (!left_node->is_start_point &&
                        right_node->neighbors_left_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                        right_node->neighbors_left[right_node->neighbors_left_count++] = left_node->id;

                    cstar_rcg_add_edge(rcg, left_node->id, right_node->id, distance);
                }
            }
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Post-Pruning Edge Generation
// -------------------------------------------------------------------------

/**
 * Connectivity check using BFS over rcg->edges (authoritative source).
 * Neighbour pointer fields (neighbor_up/down/left/right) are not consulted
 * because they may lag behind edge additions made via cstar_rcg_add_unique_edge.
 */
static bool cstar_rcg_is_connected(const cstar_rcg_t *rcg)
{
    if (rcg == NULL || rcg->node_count <= 0 || rcg->nodes == NULL)
    {
        return rcg->node_count == 0;
    }

    bool *visited = (bool *)malloc((size_t)rcg->node_count * sizeof(bool));
    if (visited == NULL)
    {
        return false;
    }

    memset(visited, 0, (size_t)rcg->node_count * sizeof(bool));

    int *queue = (int *)malloc((size_t)rcg->node_count * sizeof(int));
    if (queue == NULL)
    {
        free(visited);
        return false;
    }

    int queue_front = 0;
    int queue_back = 0;

    queue[queue_back++] = 0;
    visited[0] = true;
    int visited_count = 1;

    while (queue_front < queue_back)
    {
        int current_idx = queue[queue_front++];
        int current_node_id = rcg->nodes[current_idx].id;

        for (int e = 0; e < rcg->edge_count; ++e)
        {
            int neighbor_id = CSTAR_NO_NEIGHBOR;
            if (rcg->edges[e].node_a == current_node_id)
                neighbor_id = rcg->edges[e].node_b;
            else if (rcg->edges[e].node_b == current_node_id)
                neighbor_id = rcg->edges[e].node_a;

            if (neighbor_id == CSTAR_NO_NEIGHBOR)
                continue;

            int neighbor_idx = cstar_rcg_index_from_node_id(rcg, neighbor_id);
            if (neighbor_idx != CSTAR_NO_NEIGHBOR && !visited[neighbor_idx])
            {
                visited[neighbor_idx] = true;
                queue[queue_back++] = neighbor_idx;
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
 * For a connected planar graph: edges <= 3 * nodes - 6
 */
static bool cstar_rcg_is_planar(const cstar_rcg_t *rcg)
{
    if (rcg == NULL)
        return true;
    if (rcg->node_count < 3)
        return true;
    return rcg->edge_count <= 3 * rcg->node_count - 6;
}

static bool cstar_rcg_edge_exists(const cstar_edge_t *edges,
                                  int edge_count,
                                  int node_a,
                                  int node_b)
{
    if (edges == NULL)
        return false;
    for (int i = 0; i < edge_count; ++i)
    {
        const cstar_edge_t *edge = &edges[i];
        if ((edge->node_a == node_a && edge->node_b == node_b) ||
            (edge->node_a == node_b && edge->node_b == node_a))
            return true;
    }
    return false;
}

static void cstar_rcg_add_unique_edge(cstar_edge_t **edges,
                                      int node_a,
                                      int node_b,
                                      float cost)
{
    if (edges == NULL)
        return;
    int edge_count = (int)cvector_size(*edges);
    if (cstar_rcg_edge_exists(*edges, edge_count, node_a, node_b))
        return;
    cstar_edge_t edge = {0};
    edge.node_a = node_a;
    edge.node_b = node_b;
    edge.cost = cost;
    cvector_push_back(*edges, edge);
}

/**
 * Removes all within-lap (same lap_id) edges from rcg->edges in-place.
 *
 * Called at the start of cstar_rcg_generate_vertical_lap_edges to ensure stale
 * direct edges between existing nodes (e.g. A↔B) are purged before the
 * vertical chain is regenerated.  This prevents A* from shortcutting past
 * newly-inserted obstacle-adjacent nodes that sit between A and B.
 *
 * On the initial setup call there are no same-lap edges yet (cstar_rcg_expand_graph
 * only adds cross-lap edges), so this is a safe no-op in that case.
 */
static void cstar_rcg_remove_same_lap_edges(cstar_rcg_t *rcg)
{
    if (!rcg || !rcg->edges || rcg->edge_count == 0)
        return;

    cstar_edge_t *new_edges = NULL;
    for (int i = 0; i < rcg->edge_count; ++i)
    {
        int a_idx = cstar_rcg_index_from_node_id(rcg, rcg->edges[i].node_a);
        int b_idx = cstar_rcg_index_from_node_id(rcg, rcg->edges[i].node_b);

        /* Keep only cross-lap edges between two surviving nodes.
           Drop same-lap edges (will be regenerated below) and drop any
           edge where either endpoint was already pruned (orphan edge). */
        if (a_idx != CSTAR_NO_NEIGHBOR && b_idx != CSTAR_NO_NEIGHBOR &&
            rcg->nodes[a_idx].lap_id != rcg->nodes[b_idx].lap_id)
        {
            cvector_push_back(new_edges, rcg->edges[i]);
        }
    }

    cvector_free(rcg->edges);
    rcg->edges = new_edges;
    rcg->edge_count = (int)cvector_size(rcg->edges);
    rcg->edge_capacity = (int)cvector_capacity(rcg->edges);
}

void cstar_rcg_generate_vertical_lap_edges(cstar_rcg_t *rcg, const cstar_environment_t *env)
{
    if (!rcg || !env || !env->laps)
        return;

    /* Purge stale same-lap edges so that any previously-direct A↔B edges are
       removed before the vertical chain (A↔X, X↔B) is regenerated below.
       No-op on the initial call because no same-lap edges exist yet. */
    cstar_rcg_remove_same_lap_edges(rcg);

    cstar_lap_t *laps = (cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);

    for (int lap_index = 0; lap_index < lap_count; ++lap_index)
    {
        const cstar_lap_t *lap = &laps[lap_index];
        if (!lap->node_ids || lap->node_count <= 0)
            continue;

        // Build surviving[] from lap->node_ids, excluding start point nodes.
        // lap->node_ids is ordered bottom-to-top (ascending y); iterate in reverse
        // to process top-to-bottom and emit only downward edges (no duplicates).
        int *surviving = (int *)malloc((size_t)lap->node_count * sizeof(int));
        if (!surviving)
            continue;

        int surviving_count = 0;
        for (int i = 0; i < lap->node_count; ++i)
        {
            int node_id = lap->node_ids[i];
            int node_idx = cstar_rcg_index_from_node_id(rcg, node_id);
            if (node_idx == CSTAR_NO_NEIGHBOR)
                continue;
            const cstar_node_t *node = &rcg->nodes[node_idx];
            if (node->is_start_point)
                continue;
            surviving[surviving_count++] = node_id;
        }

        // Sort surviving[] by ascending y so that obstacle-adjacent nodes
        // appended out-of-order by cstar_generate_obstacle_adjacent_samples
        // are placed correctly in the chain before edge emission.
        for (int si = 1; si < surviving_count; ++si)
        {
            int key = surviving[si];
            int ki = cstar_rcg_index_from_node_id(rcg, key);
            float key_y = (ki != CSTAR_NO_NEIGHBOR) ? rcg->nodes[ki].pos.y : 0.0f;
            int sj = si - 1;
            while (sj >= 0)
            {
                int ci = cstar_rcg_index_from_node_id(rcg, surviving[sj]);
                float cy = (ci != CSTAR_NO_NEIGHBOR) ? rcg->nodes[ci].pos.y : 0.0f;
                if (cy <= key_y)
                    break;
                surviving[sj + 1] = surviving[sj];
                --sj;
            }
            surviving[sj + 1] = key;
        }

        // surviving[] is now in ascending y (bottom-to-top) order.
        // Iterate top-to-bottom (reverse) emitting one downward edge per node.
        for (int i = surviving_count - 1; i >= 0; --i)
        {
            int a_idx = cstar_rcg_index_from_node_id(rcg, surviving[i]);
            if (a_idx == CSTAR_NO_NEIGHBOR)
                continue;

            const cstar_node_t *a = &rcg->nodes[a_idx];

            if (a->is_top_and_bottom_end_node)
                continue;

            // Bottom end nodes have no node below them; skip (they were already
            // connected as the target of the node above).
            if (a->is_bottom_end_node)
                continue;

            // Find the first surviving node below that is not is_top_and_bottom_end_node.
            for (int j = i - 1; j >= 0; --j)
            {
                int b_idx = cstar_rcg_index_from_node_id(rcg, surviving[j]);
                if (b_idx == CSTAR_NO_NEIGHBOR)
                    continue;

                const cstar_node_t *b = &rcg->nodes[b_idx];
                if (b->is_top_and_bottom_end_node)
                    continue;

                float dx = a->pos.x - b->pos.x;
                float dy = a->pos.y - b->pos.y;
                float cost = sqrtf(dx * dx + dy * dy);
                cstar_rcg_add_unique_edge(&rcg->edges, a->id, b->id, cost);
                break;
            }
        }

        free(surviving);
    }

    // Connect the start point node to the closest surviving non-start node.
    // The start point is excluded from the lap edge loop above, so it may be
    // isolated; a nearest-neighbour edge ensures it is part of the graph.
    int start_idx = CSTAR_NO_NEIGHBOR;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (rcg->nodes[i].is_start_point)
        {
            start_idx = i;
            break;
        }
    }

    if (start_idx != CSTAR_NO_NEIGHBOR)
    {
        const cstar_node_t *start_node = &rcg->nodes[start_idx];
        int closest_id = CSTAR_NO_NEIGHBOR;
        float closest_dist = INFINITY;

        for (int i = 0; i < rcg->node_count; ++i)
        {
            if (i == start_idx)
                continue;
            const cstar_node_t *candidate = &rcg->nodes[i];
            float dx = start_node->pos.x - candidate->pos.x;
            float dy = start_node->pos.y - candidate->pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < closest_dist)
            {
                closest_dist = dist;
                closest_id = candidate->id;
            }
        }

        if (closest_id != CSTAR_NO_NEIGHBOR)
            cstar_rcg_add_unique_edge(&rcg->edges, start_node->id, closest_id, closest_dist);
    }

    rcg->edge_count = (int)cvector_size(rcg->edges);
    rcg->edge_capacity = (int)cvector_capacity(rcg->edges);

    if (!cstar_rcg_is_connected(rcg))
        LOG_WARN("RCG graph is not fully connected after pruning and vertical lap edge generation");
    if (!cstar_rcg_is_planar(rcg))
        LOG_WARN("RCG graph violates planarity constraint (Euler's formula) after pruning");
}
