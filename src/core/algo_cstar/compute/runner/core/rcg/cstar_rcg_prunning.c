
#include "cstar_rcg_prunning.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"
#include "../../../../../common/clog.h"

// Prune the RCG, leaving only end nodes and edges between them.

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

    // Allocate visited array indexed by node array position.
    bool *visited = (bool *)malloc((size_t)rcg->node_count * sizeof(bool));
    if (visited == NULL)
    {
        return false;
    }

    memset(visited, 0, (size_t)rcg->node_count * sizeof(bool));

    // BFS queue (stores node array indices).
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

        // Traverse all edges that involve this node.
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

static int cstar_rcg_index_from_node_id_in_array(const cstar_node_t *nodes,
                                                 int node_count,
                                                 int node_id)
{
    if (nodes == NULL || node_id == CSTAR_NO_NEIGHBOR)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    for (int i = 0; i < node_count; ++i)
    {
        if (nodes[i].id == node_id)
        {
            return i;
        }
    }

    return CSTAR_NO_NEIGHBOR;
}

static void cstar_rcg_reset_node_links(cstar_node_t *node)
{
    if (node == NULL)
    {
        return;
    }

    node->neighbor_up = CSTAR_NO_NEIGHBOR;
    node->neighbor_down = CSTAR_NO_NEIGHBOR;
    node->neighbors_left_count = 0;
    node->neighbors_right_count = 0;
}

static void cstar_rcg_rebuild_links_from_edges(cstar_node_t *nodes,
                                               int node_count,
                                               const cstar_edge_t *edges,
                                               int edge_count)
{
    if (nodes == NULL)
    {
        return;
    }

    for (int i = 0; i < node_count; ++i)
    {
        cstar_rcg_reset_node_links(&nodes[i]);
    }

    for (int i = 0; i < edge_count; ++i)
    {
        const cstar_edge_t *edge = &edges[i];
        int a_idx = cstar_rcg_index_from_node_id_in_array(nodes, node_count, edge->node_a);
        int b_idx = cstar_rcg_index_from_node_id_in_array(nodes, node_count, edge->node_b);
        if (a_idx == CSTAR_NO_NEIGHBOR || b_idx == CSTAR_NO_NEIGHBOR)
        {
            continue;
        }

        cstar_node_t *a = &nodes[a_idx];
        cstar_node_t *b = &nodes[b_idx];

        if (a->lap_id == b->lap_id)
        {
            if (a->pos.y <= b->pos.y)
            {
                a->neighbor_up = b->id;
                b->neighbor_down = a->id;
            }
            else
            {
                b->neighbor_up = a->id;
                a->neighbor_down = b->id;
            }
            continue;
        }

        cstar_node_t *left = a;
        cstar_node_t *right = b;
        if (left->lap_id > right->lap_id)
        {
            left = b;
            right = a;
        }

        if (left->neighbors_right_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
        {
            left->neighbors_right[left->neighbors_right_count++] = right->id;
        }
        if (right->neighbors_left_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
        {
            right->neighbors_left[right->neighbors_left_count++] = left->id;
        }
    }
}

static bool cstar_rcg_edge_exists(const cstar_edge_t *edges,
                                  int edge_count,
                                  int node_a,
                                  int node_b)
{
    if (edges == NULL)
    {
        return false;
    }

    for (int i = 0; i < edge_count; ++i)
    {
        const cstar_edge_t *edge = &edges[i];
        if ((edge->node_a == node_a && edge->node_b == node_b) ||
            (edge->node_a == node_b && edge->node_b == node_a))
        {
            return true;
        }
    }

    return false;
}

static void cstar_rcg_add_unique_edge(cstar_edge_t **edges,
                                      int node_a,
                                      int node_b,
                                      float cost)
{
    if (edges == NULL)
    {
        return;
    }

    int edge_count = (int)cvector_size(*edges);
    if (cstar_rcg_edge_exists(*edges, edge_count, node_a, node_b))
    {
        return;
    }

    cstar_edge_t edge = {0};
    edge.node_a = node_a;
    edge.node_b = node_b;
    edge.cost = cost;
    cvector_push_back(*edges, edge);
}

void cstar_rcg_prune_to_end_nodes(cstar_rcg_t *rcg)
{
    if (!rcg || !rcg->nodes)
        return;

    // Step 1: Mark non-end nodes for removal, but always keep the node at start_point
    int *keep_node = (int *)calloc(rcg->node_count, sizeof(int));
    int new_count = 0;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        int keep = 0;
        if (rcg->nodes[i].is_top_end_node || rcg->nodes[i].is_bottom_end_node)
            keep = 1;
        // Also keep node marked as start point node
        if (rcg->nodes[i].is_start_point)
            keep = 1;
        // Rule: non-end node N is essential if it is the sole cross-lap bridge
        // to an end node on an adjacent lap:
        //   - N connects to end node M on the right, and M has no other left neighbor
        //   - N connects to end node M on the left, and M has no other right neighbor
        if (!keep)
        {
            const cstar_node_t *n = &rcg->nodes[i];
            // Existing rules: sole bridge to end node
            for (int k = 0; k < n->neighbors_right_count && !keep; ++k)
            {
                int m_id = n->neighbors_right[k];
                int m_idx = cstar_rcg_index_from_node_id(rcg, m_id);
                if (m_idx != CSTAR_NO_NEIGHBOR)
                {
                    const cstar_node_t *m = &rcg->nodes[m_idx];
                    if ((m->is_top_end_node || m->is_bottom_end_node) && m->neighbors_left_count == 1)
                        keep = 1;
                }
            }
            for (int k = 0; k < n->neighbors_left_count && !keep; ++k)
            {
                int m_id = n->neighbors_left[k];
                int m_idx = cstar_rcg_index_from_node_id(rcg, m_id);
                if (m_idx != CSTAR_NO_NEIGHBOR)
                {
                    const cstar_node_t *m = &rcg->nodes[m_idx];
                    if ((m->is_top_end_node || m->is_bottom_end_node) && m->neighbors_right_count == 1)
                        keep = 1;
                }
            }

            // New rule: n is a neighbor of end node nx on an adjacent lap,
            // nx has other neighbors on n's lap (all non-end nodes),
            // and edge (n, nx) is closest to obstacle/boundary among those neighbors.
            // Check both directions (n is left or right neighbor of nx)
            for (int dir = 0; dir < 2 && !keep; ++dir)
            {
                int count = (dir == 0) ? n->neighbors_right_count : n->neighbors_left_count;
                int *adj = (dir == 0) ? n->neighbors_right : n->neighbors_left;
                for (int k = 0; k < count && !keep; ++k)
                {
                    int nx_id = adj[k];
                    int nx_idx = cstar_rcg_index_from_node_id(rcg, nx_id);
                    if (nx_idx == CSTAR_NO_NEIGHBOR)
                        continue;
                    const cstar_node_t *nx = &rcg->nodes[nx_idx];
                    if (!(nx->is_top_end_node || nx->is_bottom_end_node))
                        continue;

                    int n_lap = n->lap_id;
                    int neighbor_count = (dir == 0) ? nx->neighbors_left_count : nx->neighbors_right_count;
                    int *neighbor_ids = (dir == 0) ? nx->neighbors_left : nx->neighbors_right;
                    int found_n = 0;
                    int all_non_end = 1;
                    float min_dist = -1.0f;
                    int min_idx = -1;
                    for (int m = 0; m < neighbor_count; ++m)
                    {
                        int nb_id = neighbor_ids[m];
                        int nb_idx = cstar_rcg_index_from_node_id(rcg, nb_id);
                        if (nb_idx == CSTAR_NO_NEIGHBOR)
                            continue;
                        const cstar_node_t *nb = &rcg->nodes[nb_idx];
                        if (nb->lap_id != n_lap)
                            continue;
                        if (nb->id == n->id)
                            found_n = 1;
                        if (nb->is_top_end_node || nb->is_bottom_end_node)
                            all_non_end = 0;
                        float min_obst_dist = -1.0f;
                        for (uint32_t o = 0; o < rcg->node_count; ++o)
                        {
                            if ((int)o == nx_idx)
                                continue;
                            const cstar_node_t *ob = &rcg->nodes[o];
                            if (!(ob->is_top_end_node || ob->is_bottom_end_node))
                                continue;
                            float dx = nx->pos.x - ob->pos.x;
                            float dy = nx->pos.y - ob->pos.y;
                            float d = dx * dx + dy * dy;
                            if (min_obst_dist < 0 || d < min_obst_dist)
                                min_obst_dist = d;
                        }
                        if (min_obst_dist < 0)
                            min_obst_dist = 0.0f;
                        float edge_dist = (nb->pos.x - nx->pos.x) * (nb->pos.x - nx->pos.x) + (nb->pos.y - nx->pos.y) * (nb->pos.y - nx->pos.y);
                        float min_edge_dist = 0.0f;
                        if (min_idx != -1)
                        {
                            int min_nb_idx = cstar_rcg_index_from_node_id(rcg, neighbor_ids[min_idx]);
                            if (min_nb_idx == CSTAR_NO_NEIGHBOR)
                                continue;
                            float min_nb_x = rcg->nodes[min_nb_idx].pos.x;
                            float min_nb_y = rcg->nodes[min_nb_idx].pos.y;
                            min_edge_dist = (min_nb_x - nx->pos.x) * (min_nb_x - nx->pos.x) + (min_nb_y - nx->pos.y) * (min_nb_y - nx->pos.y);
                        }
                        int closer = 0;
                        if (min_idx == -1 || min_obst_dist < min_dist)
                            closer = 1;
                        else if (min_obst_dist == min_dist && edge_dist < min_edge_dist)
                            closer = 1;
                        if (closer)
                        {
                            min_dist = min_obst_dist;
                            min_idx = m;
                        }
                    }
                    if (found_n && all_non_end && min_idx != -1 && neighbor_ids[min_idx] == n->id)
                    {
                        keep = 1;
                    }
                }
            }
        }
        if (keep)
        {
            keep_node[i] = 1;
            ++new_count;
        }
    }

    // Step 2: Build new node array with only kept nodes
    cstar_node_t *new_nodes = NULL;
    int idx = 0;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (keep_node[i])
        {
            cvector_push_back(new_nodes, rcg->nodes[i]);
            idx++;
        }
    }

    // Step 4: Remove edges not between two kept nodes
    cstar_edge_t *new_edges = NULL;
    for (int i = 0; i < rcg->edge_count; ++i)
    {
        int a_idx = cstar_rcg_index_from_node_id(rcg, rcg->edges[i].node_a);
        int b_idx = cstar_rcg_index_from_node_id(rcg, rcg->edges[i].node_b);
        if (a_idx != CSTAR_NO_NEIGHBOR && b_idx != CSTAR_NO_NEIGHBOR && keep_node[a_idx] && keep_node[b_idx])
        {
            cvector_push_back(new_edges, rcg->edges[i]);
        }
    }

    // Step 3: Rebuild the surviving node adjacency directly from the
    // surviving edge set so intact edges remain intact after compaction.
    cstar_rcg_rebuild_links_from_edges(new_nodes, idx, new_edges, (int)cvector_size(new_edges));

    // Step 5: Replace old arrays
    cvector_free(rcg->nodes);
    rcg->nodes = new_nodes;
    rcg->node_count = (int)cvector_size(new_nodes);
    rcg->node_capacity = (int)cvector_capacity(new_nodes);

    cvector_free(rcg->edges);
    rcg->edges = new_edges;
    rcg->edge_count = (int)cvector_size(new_edges);
    rcg->edge_capacity = (int)cvector_capacity(new_edges);
    rcg->next_node_id = (rcg->next_node_id < 0) ? 0 : rcg->next_node_id;

    free(keep_node);
}

void cstar_rcg_generate_vertical_lap_edges(cstar_rcg_t *rcg, const cstar_environment_t *env)
{
    if (!rcg || !env || !env->laps)
        return;

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

        // surviving[] is in ascending y (bottom-to-top) order.
        // Iterate top-to-bottom (reverse) emitting one downward edge per node.
        for (int i = surviving_count - 1; i >= 0; --i)
        {
            int a_idx = cstar_rcg_index_from_node_id(rcg, surviving[i]);
            if (a_idx == CSTAR_NO_NEIGHBOR)
                continue;

            const cstar_node_t *a = &rcg->nodes[a_idx];

            // Skip nodes with no vertical connectivity.
            if (a->is_top_and_bottom_end_node)
                continue;

            // Bottom end nodes have no node below them; skip (they were already
            // connected as the target of the node above).
            if (a->is_bottom_end_node)
                continue;

            // Find the first surviving node below (lower index in ascending-y array)
            // that is not is_top_and_bottom_end_node.
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

    // -----------------------------------------------------------------------
    // Graph Validation (all edges finalized)
    // -----------------------------------------------------------------------
    if (!cstar_rcg_is_connected(rcg))
    {
        LOG_WARN("RCG graph is not fully connected after pruning and vertical lap edge generation");
    }
    if (!cstar_rcg_is_planar(rcg))
    {
        LOG_WARN("RCG graph violates planarity constraint (Euler's formula) after pruning");
    }
}
