/*
 * cstar_dead_end.c  --  Dead-end detection, retreat node management, A* escape
 *
 * A dead-end (Definition III.12) occurs when cstar_select_goal_node() returns
 * CSTAR_NO_NEIGHBOR: every directional neighbour of the current node is Closed
 * or absent.  The robot must navigate back to the nearest Open node before
 * coverage can resume.
 *
 * Retreat node set (Definition III.13)
 * ------------------------------------
 * N_retreat is maintained incrementally after each RCG state update:
 *   + Add any Open node within sqrt(2)*w of the current robot position.
 *   - Remove any node that has since been Closed.
 * When N_retreat is empty after a dead-end, all nodes are Closed and
 * coverage is complete.
 *
 * A* escape
 * ---------
 * The RCG edges form the A* search graph.  The heuristic h(v) is the
 * Euclidean distance from node v to the goal retreat node.  The nearest
 * retreat node (by Euclidean distance to the current robot position) is
 * selected as the A* goal.  The returned path is an ordered sequence of
 * node positions from current_node_id to the chosen retreat node.
 *
 * References: Section III.C, Definitions III.12-III.13, Algorithm 4 (lines 18-22)
 */

#include <math.h>
#include <stdlib.h>
#include "cstar_dead_end.h"

bool cstar_is_dead_end(const cstar_rcg_t *rcg, int node_id)
{
    /*
     * Pseudocode (Definition III.12):
     *   node = rcg->nodes[node_id]
     *   for dir in {up, down, left, right}:
     *     nb = node.neighbor_<dir>
     *     if nb != CSTAR_NO_NEIGHBOR && nodes[nb].state == CSTAR_NODE_OP:
     *       return false   // at least one Open neighbour exists
     *   return true
     */
    return false;
}

void cstar_retreat_update(cvector_vector_type(int) * retreat_nodes,
                          const cstar_rcg_t *rcg,
                          point_t robot_pos,
                          float w)
{
    if (retreat_nodes == NULL || rcg == NULL)
    {
        return;
    }

    float radius = sqrtf(2.0f) * w;

    // Step 1: Remove nodes that have become Closed or been pruned from the RCG.
    // Swap-with-last removal keeps the vector compact without shifting elements.
    int i = 0;
    while (i < (int)cvector_size(*retreat_nodes))
    {
        int node_id = (*retreat_nodes)[i];
        int idx = cstar_rcg_index_from_node_id(rcg, node_id);
        if (idx == CSTAR_NO_NEIGHBOR || rcg->nodes[idx].state == CSTAR_NODE_CL)
        {
            (*retreat_nodes)[i] = (*retreat_nodes)[cvector_size(*retreat_nodes) - 1];
            cvector_pop_back(*retreat_nodes);
        }
        else
        {
            i++;
        }
    }

    // Step 2: Add Open nodes within radius that are not already in the set.
    // Note: the pseudocode indexes rcg->nodes[id] using node IDs as indices,
    // which is incorrect after pruning (IDs != indices). We iterate by index.
    for (int v = 0; v < rcg->node_count; ++v)
    {
        if (rcg->nodes[v].state != CSTAR_NODE_OP)
        {
            continue;
        }

        float dx = robot_pos.x - rcg->nodes[v].pos.x;
        float dy = robot_pos.y - rcg->nodes[v].pos.y;
        if (sqrtf(dx * dx + dy * dy) > radius)
        {
            continue;
        }

        int node_id = rcg->nodes[v].id;
        bool already_present = false;
        for (int j = 0; j < (int)cvector_size(*retreat_nodes); ++j)
        {
            if ((*retreat_nodes)[j] == node_id)
            {
                already_present = true;
                break;
            }
        }

        if (!already_present)
        {
            cvector_push_back(*retreat_nodes, node_id);
        }
    }
}

int cstar_escape_dead_end(const cstar_rcg_t *rcg,
                          int current_node_id,
                          const cvector_vector_type(int) retreat_nodes,
                          cvector_vector_type(point_t) * path_out)
{
    if (rcg == NULL || retreat_nodes == NULL || cvector_size(retreat_nodes) == 0)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    int start_idx = cstar_rcg_index_from_node_id(rcg, current_node_id);
    if (start_idx == CSTAR_NO_NEIGHBOR)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    // Select the nearest retreat node (by Euclidean distance from the current
    // position) as the A* goal. Note: pseudocode uses node IDs as indices —
    // corrected here to use cstar_rcg_index_from_node_id.
    point_t start_pos = rcg->nodes[start_idx].pos;
    int goal_id = CSTAR_NO_NEIGHBOR;
    int goal_idx = CSTAR_NO_NEIGHBOR;
    float min_dist = INFINITY;

    int retreat_count = (int)cvector_size(retreat_nodes);
    for (int i = 0; i < retreat_count; ++i)
    {
        int rid = retreat_nodes[i];
        int ridx = cstar_rcg_index_from_node_id(rcg, rid);
        if (ridx == CSTAR_NO_NEIGHBOR)
        {
            continue;
        }
        float dx = start_pos.x - rcg->nodes[ridx].pos.x;
        float dy = start_pos.y - rcg->nodes[ridx].pos.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < min_dist)
        {
            min_dist = d;
            goal_id = rid;
            goal_idx = ridx;
        }
    }

    if (goal_id == CSTAR_NO_NEIGHBOR)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    int n = rcg->node_count;

    // A* working arrays indexed by node index (not ID).
    float *g_score = (float *)malloc((size_t)n * sizeof(float));
    int *came_from = (int *)malloc((size_t)n * sizeof(int));
    bool *in_open = (bool *)calloc((size_t)n, sizeof(bool));
    bool *closed = (bool *)calloc((size_t)n, sizeof(bool));

    if (g_score == NULL || came_from == NULL || in_open == NULL || closed == NULL)
    {
        free(g_score);
        free(came_from);
        free(in_open);
        free(closed);
        return CSTAR_NO_NEIGHBOR;
    }

    for (int i = 0; i < n; ++i)
    {
        g_score[i] = INFINITY;
        came_from[i] = CSTAR_NO_NEIGHBOR;
    }
    g_score[start_idx] = 0.0f;
    in_open[start_idx] = true;

    point_t goal_pos = rcg->nodes[goal_idx].pos;

    // A* main loop: linear scan for minimum f = g + h in the open set.
    // Linear scan is adequate for the sparse RCG (tens to low hundreds of nodes).
    for (;;)
    {
        int curr_idx = CSTAR_NO_NEIGHBOR;
        float curr_f = INFINITY;
        for (int i = 0; i < n; ++i)
        {
            if (!in_open[i])
            {
                continue;
            }
            float dx = rcg->nodes[i].pos.x - goal_pos.x;
            float dy = rcg->nodes[i].pos.y - goal_pos.y;
            float f = g_score[i] + sqrtf(dx * dx + dy * dy);
            if (f < curr_f)
            {
                curr_f = f;
                curr_idx = i;
            }
        }

        if (curr_idx == CSTAR_NO_NEIGHBOR)
        {
            // Open set exhausted — goal is unreachable (RCG connectivity violation).
            break;
        }

        if (curr_idx == goal_idx)
        {
            break;
        }

        in_open[curr_idx] = false;
        closed[curr_idx] = true;

        // Expand all directional neighbors encoded in the node struct.
        const cstar_node_t *curr_node = &rcg->nodes[curr_idx];
        int neighbor_ids[2 + CSTAR_MAX_CROSS_LAP_NEIGHBORS * 2];
        int neighbor_count = 0;

        if (curr_node->neighbor_up != CSTAR_NO_NEIGHBOR)
        {
            neighbor_ids[neighbor_count++] = curr_node->neighbor_up;
        }
        if (curr_node->neighbor_down != CSTAR_NO_NEIGHBOR)
        {
            neighbor_ids[neighbor_count++] = curr_node->neighbor_down;
        }
        for (int k = 0; k < curr_node->neighbors_left_count; ++k)
        {
            neighbor_ids[neighbor_count++] = curr_node->neighbors_left[k];
        }
        for (int k = 0; k < curr_node->neighbors_right_count; ++k)
        {
            neighbor_ids[neighbor_count++] = curr_node->neighbors_right[k];
        }

        for (int k = 0; k < neighbor_count; ++k)
        {
            int nb_idx = cstar_rcg_index_from_node_id(rcg, neighbor_ids[k]);
            if (nb_idx == CSTAR_NO_NEIGHBOR || closed[nb_idx])
            {
                continue;
            }
            float dx = rcg->nodes[curr_idx].pos.x - rcg->nodes[nb_idx].pos.x;
            float dy = rcg->nodes[curr_idx].pos.y - rcg->nodes[nb_idx].pos.y;
            float tentative_g = g_score[curr_idx] + sqrtf(dx * dx + dy * dy);
            if (tentative_g < g_score[nb_idx])
            {
                came_from[nb_idx] = curr_idx;
                g_score[nb_idx] = tentative_g;
                in_open[nb_idx] = true;
            }
        }
    }

    // Path reconstruction: walk came_from[] backwards from goal to start,
    // collect positions in reverse, then emit in forward order.
    if (path_out != NULL)
    {
        *path_out = NULL;
        bool goal_reached = (goal_idx == start_idx) ||
                            (came_from[goal_idx] != CSTAR_NO_NEIGHBOR);
        if (goal_reached)
        {
            cvector_vector_type(point_t) rev_path = NULL;
            int node = goal_idx;
            while (node != CSTAR_NO_NEIGHBOR)
            {
                cvector_push_back(rev_path, rcg->nodes[node].pos);
                if (node == start_idx)
                {
                    break;
                }
                node = came_from[node];
            }
            int rev_len = (int)cvector_size(rev_path);
            for (int i = rev_len - 1; i >= 0; --i)
            {
                cvector_push_back(*path_out, rev_path[i]);
            }
            cvector_free(rev_path);
        }
    }

    free(g_score);
    free(came_from);
    free(in_open);
    free(closed);
    return goal_id;
}
