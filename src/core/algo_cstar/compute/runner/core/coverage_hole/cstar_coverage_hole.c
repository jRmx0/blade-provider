/*
 * cstar_coverage_hole.c  --  Coverage hole detection and TSP trajectory (Algorithm 3)
 *
 * A coverage hole (Definition I.1) is a connected sub-region of the environment
 * that the robot cannot reach in the normal traversal order because it has been
 * enclosed by Closed nodes and obstacles.
 *
 * Hole detection
 * --------------
 * After every state update (Algorithm 2) a BFS flood-fill is launched from each
 * unlabelled Open neighbour of the current node.  A component is declared a hole
 * when the flood-fill cannot reach the unknown area or the goal node.
 * The goal node acts as a stop boundary so the robot's intended path is never
 * included in a hole.
 *
 * TSP trajectory (Algorithm 3)
 * ----------------------------
 * When a hole is found, its Open nodes are densified to spacing w, the resulting
 * set is solved as a TSP (Hamiltonian path), and the tour is returned as an
 * ordered list of node indices:
 *   1. Nearest-neighbour heuristic builds an initial tour (O(n^2)).
 *   2. 2-opt improvement iteratively removes crossing edges (O(n^2) per pass).
 * The robot visits all hole nodes before resuming normal coverage.
 *
 * References: Section III.E, Definition I.1, Algorithm 3,
 *             Algorithm 4 (lines 13-17)
 */

#include <stdlib.h>
#include <math.h>
#include "cstar_coverage_hole.h"

/* -------------------------------------------------------------------------
 * Internal helper: euclidean distance between two RCG nodes.
 * Returns 0 when either node ID is not found.
 * ------------------------------------------------------------------------- */
static float cstar_hole_node_dist(const cstar_rcg_t *rcg, int id_a, int id_b)
{
    const cstar_node_t *a = cstar_rcg_get_node_by_id(rcg, id_a);
    const cstar_node_t *b = cstar_rcg_get_node_by_id(rcg, id_b);
    if (a == NULL || b == NULL)
        return 0.0f;
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    return sqrtf(dx * dx + dy * dy);
}

cvector_vector_type(cvector_vector_type(int))
    cstar_detect_coverage_holes(const cstar_rcg_t *rcg,
                                int current_node_id,
                                int goal_node_id,
                                float w,
                                const cstar_environment_t *env)
{
    (void)w;
    (void)env;

    cvector_vector_type(cvector_vector_type(int)) holes = NULL;

    if (rcg == NULL || rcg->node_count == 0)
        return holes;

    int current_idx = cstar_rcg_index_from_node_id(rcg, current_node_id);
    if (current_idx == CSTAR_NO_NEIGHBOR)
        return holes;

    /* visited[] is indexed by node array index (not node ID) */
    bool *visited = (bool *)calloc((size_t)rcg->node_count, sizeof(bool));
    if (visited == NULL)
        return holes;

    visited[current_idx] = true;

    int goal_idx = cstar_rcg_index_from_node_id(rcg, goal_node_id);
    if (goal_idx != CSTAR_NO_NEIGHBOR)
        visited[goal_idx] = true;

    const cstar_node_t *cur = &rcg->nodes[current_idx];

    /* Gather all directional neighbor IDs of current node as BFS seed candidates */
    int seed_candidates[2 + CSTAR_MAX_CROSS_LAP_NEIGHBORS * 2];
    int seed_candidate_count = 0;

    seed_candidates[seed_candidate_count++] = cur->neighbor_up;
    seed_candidates[seed_candidate_count++] = cur->neighbor_down;
    for (int i = 0; i < cur->neighbors_left_count; i++)
        seed_candidates[seed_candidate_count++] = cur->neighbors_left[i];
    for (int i = 0; i < cur->neighbors_right_count; i++)
        seed_candidates[seed_candidate_count++] = cur->neighbors_right[i];

    /* Run one BFS per unvisited, Open seed — each produces a candidate hole component */
    for (int s = 0; s < seed_candidate_count; s++)
    {
        int seed_id = seed_candidates[s];
        if (seed_id == CSTAR_NO_NEIGHBOR)
            continue;

        int seed_idx = cstar_rcg_index_from_node_id(rcg, seed_id);
        if (seed_idx == CSTAR_NO_NEIGHBOR || visited[seed_idx])
            continue;

        visited[seed_idx] = true;

        /* Skip Closed seeds — they are boundaries, not component members */
        if (rcg->nodes[seed_idx].state == CSTAR_NODE_CL)
            continue;

        /* BFS from seed_id */
        cvector_vector_type(int) component = NULL;
        cvector_vector_type(int) bfs_queue = NULL;
        bool is_hole = true;
        int bfs_head = 0;

        cvector_push_back(bfs_queue, seed_id);

        while (bfs_head < (int)cvector_size(bfs_queue))
        {
            int v_id = bfs_queue[bfs_head++];
            cvector_push_back(component, v_id);

            int v_idx = cstar_rcg_index_from_node_id(rcg, v_id);
            if (v_idx == CSTAR_NO_NEIGHBOR)
                continue;

            const cstar_node_t *v = &rcg->nodes[v_idx];

            /* Collect all neighbor IDs for this node */
            int nb_ids[2 + CSTAR_MAX_CROSS_LAP_NEIGHBORS * 2];
            int nb_count = 0;
            nb_ids[nb_count++] = v->neighbor_up;
            nb_ids[nb_count++] = v->neighbor_down;
            for (int li = 0; li < v->neighbors_left_count; li++)
                nb_ids[nb_count++] = v->neighbors_left[li];
            for (int ri = 0; ri < v->neighbors_right_count; ri++)
                nb_ids[nb_count++] = v->neighbors_right[ri];

            for (int ni = 0; ni < nb_count; ni++)
            {
                int nb_id = nb_ids[ni];
                if (nb_id == CSTAR_NO_NEIGHBOR)
                    continue;

                /* Component touches goal → connected to main coverage trajectory.
                 * After the current→goal move the robot can reach these nodes,
                 * so this component is not yet a hole. */
                if (nb_id == goal_node_id)
                {
                    is_hole = false;
                    continue;
                }

                int nb_idx = cstar_rcg_index_from_node_id(rcg, nb_id);
                if (nb_idx == CSTAR_NO_NEIGHBOR || visited[nb_idx])
                    continue;

                visited[nb_idx] = true;

                /* Closed neighbors act as flood-fill boundaries; mark visited
                 * so they are not re-seeded, but do not enqueue them. */
                if (rcg->nodes[nb_idx].state == CSTAR_NODE_CL)
                    continue;

                cvector_push_back(bfs_queue, nb_id);
            }
        }

        cvector_free(bfs_queue);

        if (is_hole && cvector_size(component) > 0)
        {
            cvector_push_back(holes, component);
        }
        else
        {
            cvector_free(component);
        }
    }

    free(visited);
    return holes;
}

cvector_vector_type(int)
    cstar_compute_tsp_trajectory(cstar_rcg_t *rcg,
                                 int current_node_id,
                                 int goal_node_id,
                                 const int *hole_nodes,
                                 int hole_count,
                                 float w,
                                 const cstar_environment_t *env)
{
    (void)w;
    (void)env;

    if (hole_count <= 0 || hole_nodes == NULL || rcg == NULL)
        return NULL;

    /* Determine TSP end node (Algorithm 3):
     *  Condition 1: goal has an Open neighbor not in the hole → n_e = goal_id
     *  Condition 2: current has an Open neighbor not in the hole → n_e = current_node_id
     *  Fallback: n_e = goal_id */

    bool cond1 = false;

    const cstar_node_t *goal_node = cstar_rcg_get_node_by_id(rcg, goal_node_id);
    if (goal_node != NULL)
    {
        int gnb[2 + CSTAR_MAX_CROSS_LAP_NEIGHBORS * 2];
        int gnb_count = 0;
        gnb[gnb_count++] = goal_node->neighbor_up;
        gnb[gnb_count++] = goal_node->neighbor_down;
        for (int i = 0; i < goal_node->neighbors_left_count; i++)
            gnb[gnb_count++] = goal_node->neighbors_left[i];
        for (int i = 0; i < goal_node->neighbors_right_count; i++)
            gnb[gnb_count++] = goal_node->neighbors_right[i];

        for (int ni = 0; ni < gnb_count && !cond1; ni++)
        {
            int nb_id = gnb[ni];
            if (nb_id == CSTAR_NO_NEIGHBOR || nb_id == current_node_id)
                continue;
            const cstar_node_t *nb = cstar_rcg_get_node_by_id(rcg, nb_id);
            if (nb == NULL || nb->state != CSTAR_NODE_OP)
                continue;
            bool in_hole = false;
            for (int h = 0; h < hole_count && !in_hole; h++)
                if (hole_nodes[h] == nb_id)
                    in_hole = true;
            if (!in_hole)
                cond1 = true;
        }
    }

    int n_e;
    if (cond1)
    {
        n_e = goal_node_id;
    }
    else
    {
        bool cond2 = false;
        const cstar_node_t *cur_node = cstar_rcg_get_node_by_id(rcg, current_node_id);
        if (cur_node != NULL)
        {
            int cnb[2 + CSTAR_MAX_CROSS_LAP_NEIGHBORS * 2];
            int cnb_count = 0;
            cnb[cnb_count++] = cur_node->neighbor_up;
            cnb[cnb_count++] = cur_node->neighbor_down;
            for (int i = 0; i < cur_node->neighbors_left_count; i++)
                cnb[cnb_count++] = cur_node->neighbors_left[i];
            for (int i = 0; i < cur_node->neighbors_right_count; i++)
                cnb[cnb_count++] = cur_node->neighbors_right[i];

            for (int ni = 0; ni < cnb_count && !cond2; ni++)
            {
                int nb_id = cnb[ni];
                if (nb_id == CSTAR_NO_NEIGHBOR || nb_id == goal_node_id)
                    continue;
                const cstar_node_t *nb = cstar_rcg_get_node_by_id(rcg, nb_id);
                if (nb == NULL || nb->state != CSTAR_NODE_OP)
                    continue;
                bool in_hole = false;
                for (int h = 0; h < hole_count && !in_hole; h++)
                    if (hole_nodes[h] == nb_id)
                        in_hole = true;
                if (!in_hole)
                    cond2 = true;
            }
        }
        n_e = cond2 ? current_node_id : goal_node_id;
    }

    /* Build TSP node set: current_node_id + hole_nodes + n_e (if goal) — deduplicated */
    int tsp_capacity = 1 + hole_count + 1; /* worst-case with goal appended */
    int *tsp_ids = (int *)malloc((size_t)tsp_capacity * sizeof(int));
    if (tsp_ids == NULL)
        return NULL;

    tsp_ids[0] = current_node_id;
    int tsp_count = 1;
    for (int i = 0; i < hole_count; i++)
    {
        if (hole_nodes[i] != current_node_id)
            tsp_ids[tsp_count++] = hole_nodes[i];
    }

    /* Include goal as final waypoint when Condition 1 holds */
    if (n_e == goal_node_id)
    {
        bool already = false;
        for (int i = 0; i < tsp_count && !already; i++)
            if (tsp_ids[i] == goal_node_id)
                already = true;
        if (!already)
            tsp_ids[tsp_count++] = goal_node_id;
    }

    if (tsp_count <= 1)
    {
        free(tsp_ids);
        return NULL;
    }

    /* Nearest-neighbour tour starting at current_node_id */
    cvector_vector_type(int) tour =
        cstar_tsp_nearest_neighbour(tsp_ids, tsp_count, current_node_id, rcg);
    free(tsp_ids);

    if (tour == NULL)
        return NULL;

    /* 2-opt improvement */
    int tour_size = (int)cvector_size(tour);
    if (tour_size >= 4)
        cstar_tsp_2opt(tour, tour_size, rcg);

    return tour;
}

cvector_vector_type(int) cstar_tsp_nearest_neighbour(const int *node_ids,
                                                     int count,
                                                     int start_id,
                                                     const cstar_rcg_t *rcg)
{
    if (node_ids == NULL || count <= 0 || rcg == NULL)
        return NULL;

    bool *visited = (bool *)calloc((size_t)count, sizeof(bool));
    if (visited == NULL)
        return NULL;

    cvector_vector_type(int) tour = NULL;

    /* Find start_id in node_ids; fall back to index 0 if not found */
    int current_list_idx = 0;
    for (int i = 0; i < count; i++)
    {
        if (node_ids[i] == start_id)
        {
            current_list_idx = i;
            break;
        }
    }

    visited[current_list_idx] = true;
    cvector_push_back(tour, node_ids[current_list_idx]);

    for (int step = 1; step < count; step++)
    {
        const cstar_node_t *cur_node = cstar_rcg_get_node_by_id(rcg, node_ids[current_list_idx]);
        if (cur_node == NULL)
            break;

        /* Use squared distance to avoid sqrtf in the inner loop */
        float best_sq = 1e30f;
        int best_idx = -1;

        for (int i = 0; i < count; i++)
        {
            if (visited[i])
                continue;
            const cstar_node_t *cand = cstar_rcg_get_node_by_id(rcg, node_ids[i]);
            if (cand == NULL)
                continue;
            float dx = cand->pos.x - cur_node->pos.x;
            float dy = cand->pos.y - cur_node->pos.y;
            float sq = dx * dx + dy * dy;
            if (sq < best_sq)
            {
                best_sq = sq;
                best_idx = i;
            }
        }

        if (best_idx == -1)
            break;

        visited[best_idx] = true;
        current_list_idx = best_idx;
        cvector_push_back(tour, node_ids[current_list_idx]);
    }

    free(visited);
    return tour;
}

void cstar_tsp_2opt(int *tour_nodes, int count, const cstar_rcg_t *rcg)
{
    /* Requires at least 4 nodes for a meaningful 2-opt swap */
    if (tour_nodes == NULL || count < 4 || rcg == NULL)
        return;

    const float epsilon = 1e-6f;
    bool improved = true;

    while (improved)
    {
        improved = false;
        for (int i = 0; i < count - 1; i++)
        {
            for (int k = i + 1; k < count; k++)
            {
                /* Cost of current edges (i→i+1) and (k→k+1 mod count) */
                float d_old =
                    cstar_hole_node_dist(rcg, tour_nodes[i], tour_nodes[i + 1]) +
                    cstar_hole_node_dist(rcg, tour_nodes[k], tour_nodes[(k + 1) % count]);

                /* Cost after reversing segment [i+1 .. k] */
                float d_new =
                    cstar_hole_node_dist(rcg, tour_nodes[i], tour_nodes[k]) +
                    cstar_hole_node_dist(rcg, tour_nodes[i + 1], tour_nodes[(k + 1) % count]);

                if (d_new < d_old - epsilon)
                {
                    /* Reverse tour_nodes[i+1 .. k] in-place */
                    int lo = i + 1, hi = k;
                    while (lo < hi)
                    {
                        int tmp = tour_nodes[lo];
                        tour_nodes[lo] = tour_nodes[hi];
                        tour_nodes[hi] = tmp;
                        lo++;
                        hi--;
                    }
                    improved = true;
                }
            }
        }
    }
}
