
#include "cstar_rcg_prunning.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// Prune the RCG, leaving only end nodes and edges between them.

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

    // Same-lap chains may lose intermediate nodes during pruning. Reconnect the
    // surviving neighbors so vertical lap edges continue through the pruned gap.
    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (!keep_node[i])
        {
            continue;
        }

        const cstar_node_t *node = &rcg->nodes[i];
        int next_id = node->neighbor_up;

        while (next_id != CSTAR_NO_NEIGHBOR)
        {
            int next_idx = cstar_rcg_index_from_node_id(rcg, next_id);
            if (next_idx == CSTAR_NO_NEIGHBOR)
            {
                break;
            }

            if (keep_node[next_idx])
            {
                const cstar_node_t *next_node = &rcg->nodes[next_idx];
                float dx = node->pos.x - next_node->pos.x;
                float dy = node->pos.y - next_node->pos.y;
                float cost = sqrtf(dx * dx + dy * dy);
                cstar_rcg_add_unique_edge(&new_edges, node->id, next_node->id, cost);
                break;
            }

            next_id = rcg->nodes[next_idx].neighbor_up;
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
