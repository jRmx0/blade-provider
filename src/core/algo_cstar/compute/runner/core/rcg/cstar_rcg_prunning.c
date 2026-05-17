
#include "cstar_rcg_prunning.h"
#include <stdlib.h>
#include <stdbool.h>
#include "../../../../cstar.h"
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
                if (m_id >= 0 && m_id < rcg->node_count)
                {
                    const cstar_node_t *m = &rcg->nodes[m_id];
                    if (m->is_end_node && m->neighbors_left_count == 1)
                        keep = 1;
                }
            }
            for (int k = 0; k < n->neighbors_left_count && !keep; ++k)
            {
                int m_id = n->neighbors_left[k];
                if (m_id >= 0 && m_id < rcg->node_count)
                {
                    const cstar_node_t *m = &rcg->nodes[m_id];
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
                    if (nx_id < 0 || nx_id >= rcg->node_count)
                        continue;
                    const cstar_node_t *nx = &rcg->nodes[nx_id];
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
                        if (nb_id < 0 || nb_id >= rcg->node_count)
                            continue;
                        const cstar_node_t *nb = &rcg->nodes[nb_id];
                        if (nb->lap_id != n_lap)
                            continue;
                        if (nb_id == i)
                            found_n = 1;
                        if (nb->is_end_node)
                            all_non_end = 0;
                        float min_obst_dist = -1.0f;
                        for (uint32_t o = 0; o < rcg->node_count; ++o)
                        {
                            if (o == nx_id)
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
                            float min_nb_x = rcg->nodes[neighbor_ids[min_idx]].pos.x;
                            float min_nb_y = rcg->nodes[neighbor_ids[min_idx]].pos.y;
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
                    if (found_n && all_non_end && min_idx != -1 && neighbor_ids[min_idx] == i)
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
    int *old_to_new = (int *)malloc(rcg->node_count * sizeof(int));
    int idx = 0;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (keep_node[i])
        {
            cvector_push_back(new_nodes, rcg->nodes[i]);
            old_to_new[i] = idx++;
        }
        else
        {
            old_to_new[i] = -1;
        }
    }

    // Step 3: Remove edges not between two kept nodes
    cstar_edge_t *new_edges = NULL;
    for (int i = 0; i < rcg->edge_count; ++i)
    {
        int a = rcg->edges[i].node_a;
        int b = rcg->edges[i].node_b;
        if (a >= 0 && b >= 0 && keep_node[a] && keep_node[b])
        {
            // Remap node indices
            cstar_edge_t e = rcg->edges[i];
            e.node_a = old_to_new[a];
            e.node_b = old_to_new[b];
            cvector_push_back(new_edges, e);
        }
    }

    // Step 4: Replace old arrays
    cvector_free(rcg->nodes);
    rcg->nodes = new_nodes;
    rcg->node_count = (int)cvector_size(new_nodes);
    rcg->node_capacity = (int)cvector_capacity(new_nodes);

    cvector_free(rcg->edges);
    rcg->edges = new_edges;
    rcg->edge_count = (int)cvector_size(new_edges);
    rcg->edge_capacity = (int)cvector_capacity(new_edges);

    free(keep_node);
    free(old_to_new);
}
