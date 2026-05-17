
#include "cstar_rcg_prunning.h"
#include <stdlib.h>
#include <stdbool.h>
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// Prune the RCG, leaving only end nodes and edges between them.

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
        if (rcg->nodes[i].is_end_node)
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
                    if (m->is_end_node && m->neighbors_left_count == 1)
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
                    if (m->is_end_node && m->neighbors_right_count == 1)
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
                    if (!nx->is_end_node)
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
                        if (nb->is_end_node)
                            all_non_end = 0;
                        float min_obst_dist = -1.0f;
                        for (uint32_t o = 0; o < rcg->node_count; ++o)
                        {
                            if ((int)o == nx_idx)
                                continue;
                            const cstar_node_t *ob = &rcg->nodes[o];
                            if (!ob->is_end_node)
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

    // Step 3: Remove adjacency references to pruned nodes (IDs are preserved).
    for (int i = 0; i < idx; ++i)
    {
        cstar_node_t *node = &new_nodes[i];

        int up_idx = cstar_rcg_index_from_node_id(rcg, node->neighbor_up);
        if (up_idx == CSTAR_NO_NEIGHBOR || !keep_node[up_idx])
            node->neighbor_up = CSTAR_NO_NEIGHBOR;

        int down_idx = cstar_rcg_index_from_node_id(rcg, node->neighbor_down);
        if (down_idx == CSTAR_NO_NEIGHBOR || !keep_node[down_idx])
            node->neighbor_down = CSTAR_NO_NEIGHBOR;

        int left_write = 0;
        for (int k = 0; k < node->neighbors_left_count; ++k)
        {
            int neighbor_id = node->neighbors_left[k];
            int neighbor_idx = cstar_rcg_index_from_node_id(rcg, neighbor_id);
            if (neighbor_idx != CSTAR_NO_NEIGHBOR && keep_node[neighbor_idx])
            {
                node->neighbors_left[left_write++] = neighbor_id;
            }
        }
        node->neighbors_left_count = left_write;

        int right_write = 0;
        for (int k = 0; k < node->neighbors_right_count; ++k)
        {
            int neighbor_id = node->neighbors_right[k];
            int neighbor_idx = cstar_rcg_index_from_node_id(rcg, neighbor_id);
            if (neighbor_idx != CSTAR_NO_NEIGHBOR && keep_node[neighbor_idx])
            {
                node->neighbors_right[right_write++] = neighbor_id;
            }
        }
        node->neighbors_right_count = right_write;
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
