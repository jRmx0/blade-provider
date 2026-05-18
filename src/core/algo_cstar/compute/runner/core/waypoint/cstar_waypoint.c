/*
 * cstar_waypoint.c  --  Goal node selection (Algorithm 1) and state update (Algorithm 2)
 *
 * Per-iteration waypoint decisions (Section III.B):
 *
 * Algorithm 1 - Goal Node Selection
 * ----------------------------------
 * The robot always prefers to continue along its current lap (left neighbour),
 * then to switch to the lap above (up) or below (down), and only reverses to
 * the right lap as a last resort.  Priority:
 *
 *   left -> up -> down -> right
 *
 * When multiple Open candidates exist on the left or right lap, one is picked
 * at random to avoid systematic bias.  If all four slots are Closed or absent,
 * the function returns CSTAR_NO_NEIGHBOR and the caller must invoke the dead-end
 * escape strategy (cstar_escape_dead_end).
 *
 * Algorithm 2 - State Update
 * --------------------------
 * After a goal is selected, the current node is closed UNLESS both same-lap
 * neighbours (up and down) are Open.  Closing a node between two Open same-lap
 * neighbours would split an Open run and require an extra escape later.
 *
 * When closing during a left-lap transition, link nodes are inserted at
 * distance w between the current node and any Open same-lap neighbour whose
 * distance exceeds w, so traversability is preserved.
 *
 * References: Section III.B, Algorithms 1-2
 */

#include "cstar_waypoint.h"
#include "../rcg/cstar_rcg.h"
#include "../rcg/cstar_rcg_growth.h"
#include <math.h>
#include <stdlib.h>

int cstar_select_goal_node(const cstar_rcg_t *rcg, int current_node_id)
{
    if (rcg == NULL)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    const cstar_node_t *node = cstar_rcg_get_node_by_id(rcg, current_node_id);
    if (node == NULL)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    // Priority 1: left lap — collect all Open candidates, pick one at random.
    int left_candidates[CSTAR_MAX_CROSS_LAP_NEIGHBORS];
    int left_count = 0;
    for (int i = 0; i < node->neighbors_left_count; ++i)
    {
        int left_id = node->neighbors_left[i];
        const cstar_node_t *left = cstar_rcg_get_node_by_id(rcg, left_id);
        if (left != NULL && left->state == CSTAR_NODE_OP)
        {
            left_candidates[left_count++] = left_id;
        }
    }
    if (left_count > 0)
    {
        return left_candidates[rand() % left_count];
    }

    // Priority 2: up (single slot — no randomness needed).
    int up_id = node->neighbor_up;
    const cstar_node_t *up = cstar_rcg_get_node_by_id(rcg, up_id);
    if (up_id != CSTAR_NO_NEIGHBOR && up != NULL && up->state == CSTAR_NODE_OP)
    {
        return up_id;
    }

    // Priority 3: down (single slot — no randomness needed).
    int down_id = node->neighbor_down;
    const cstar_node_t *down = cstar_rcg_get_node_by_id(rcg, down_id);
    if (down_id != CSTAR_NO_NEIGHBOR && down != NULL && down->state == CSTAR_NODE_OP)
    {
        return down_id;
    }

    // Priority 4: right lap — collect all Open candidates, pick one at random.
    int right_candidates[CSTAR_MAX_CROSS_LAP_NEIGHBORS];
    int right_count = 0;
    for (int i = 0; i < node->neighbors_right_count; ++i)
    {
        int right_id = node->neighbors_right[i];
        const cstar_node_t *right = cstar_rcg_get_node_by_id(rcg, right_id);
        if (right != NULL && right->state == CSTAR_NODE_OP)
        {
            right_candidates[right_count++] = right_id;
        }
    }
    if (right_count > 0)
    {
        return right_candidates[rand() % right_count];
    }

    return CSTAR_NO_NEIGHBOR;
}

int cstar_update_node_state(cstar_rcg_t *rcg,
                            int current_node_id,
                            int goal_node_id,
                            float w)
{
    if (rcg == NULL)
    {
        return 0;
    }

    const cstar_node_t *node = cstar_rcg_get_node_by_id(rcg, current_node_id);
    if (node == NULL)
    {
        return 0;
    }

    // --- Determine if current node can be closed (Algorithm 2) ---
    // Do NOT close if both same-lap neighbours are Open: closing would split
    // an Open run on the lap and force an unnecessary dead-end escape later.
    int up_id = node->neighbor_up;
    int down_id = node->neighbor_down;

    const cstar_node_t *up = cstar_rcg_get_node_by_id(rcg, up_id);
    const cstar_node_t *down = cstar_rcg_get_node_by_id(rcg, down_id);

    bool up_open = (up_id != CSTAR_NO_NEIGHBOR && up != NULL && up->state == CSTAR_NODE_OP);
    bool down_open = (down_id != CSTAR_NO_NEIGHBOR && down != NULL && down->state == CSTAR_NODE_OP);

    bool can_close = !(up_open && down_open);
    if (!can_close)
    {
        return 0;
    }

    // --- Determine if the goal is on the left lap ---
    bool goal_is_left = false;
    for (int i = 0; i < node->neighbors_left_count; ++i)
    {
        if (node->neighbors_left[i] == goal_node_id)
        {
            goal_is_left = true;
            break;
        }
    }

    int link_nodes_created = 0;

    // --- Insert link nodes (left-lap transitions only) ---
    // When closing during a left-lap transition, a distant Open same-lap
    // neighbour loses its only traversal path through current.  A link node
    // is placed at distance w so the neighbour stays reachable.
    if (goal_is_left)
    {
        // Process up neighbour, then down neighbour.
        int dirs[2] = {up_id, down_id};
        bool opens[2] = {up_open, down_open};
        bool is_up[2] = {true, false};

        for (int d = 0; d < 2; ++d)
        {
            if (!opens[d])
            {
                continue;
            }

            int nbr_id = dirs[d];

            // Re-fetch current and neighbour every iteration: earlier
            // cstar_rcg_add_node calls may have reallocated rcg->nodes.
            const cstar_node_t *cur = cstar_rcg_get_node_by_id(rcg, current_node_id);
            const cstar_node_t *nbr = cstar_rcg_get_node_by_id(rcg, nbr_id);
            if (cur == NULL || nbr == NULL)
            {
                continue;
            }

            float dx = nbr->pos.x - cur->pos.x;
            float dy = nbr->pos.y - cur->pos.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist <= w)
            {
                // Neighbour is already within w — no link node needed.
                continue;
            }

            // Place link node at distance w from current toward neighbour.
            float scale = w / dist;
            point_t link_pos = {cur->pos.x + dx * scale,
                                cur->pos.y + dy * scale};

            int link_id = cstar_rcg_add_node(rcg, link_pos,
                                             cur->lap_id,
                                             false, false, false, false);
            if (link_id == CSTAR_NO_NEIGHBOR)
            {
                continue;
            }

            // Mark as link node. Re-fetch: add_node may have reallocated.
            cstar_node_t *link_mut = cstar_rcg_get_node_by_id_mut(rcg, link_id);
            if (link_mut == NULL)
            {
                continue;
            }
            link_mut->is_link_node = true;

            // Wire same-lap neighbor pointers through the link.
            // Re-fetch current and nbr again after add_node reallocation.
            cstar_node_t *cur_mut = cstar_rcg_get_node_by_id_mut(rcg, current_node_id);
            cstar_node_t *nbr_mut = cstar_rcg_get_node_by_id_mut(rcg, nbr_id);

            if (is_up[d])
            {
                // current --up--> link --up--> nbr
                if (cur_mut)
                {
                    cur_mut->neighbor_up = link_id;
                }
                link_mut->neighbor_down = current_node_id;
                link_mut->neighbor_up = nbr_id;
                if (nbr_mut)
                {
                    nbr_mut->neighbor_down = link_id;
                }
            }
            else
            {
                // current --down--> link --down--> nbr
                if (cur_mut)
                {
                    cur_mut->neighbor_down = link_id;
                }
                link_mut->neighbor_up = current_node_id;
                link_mut->neighbor_down = nbr_id;
                if (nbr_mut)
                {
                    nbr_mut->neighbor_up = link_id;
                }
            }

            // Add edges: current <-> link, link <-> nbr.
            // The gap between current and its same-lap neighbour is already
            // obstacle-free, so no collision check is required.
            cstar_rcg_add_edge(rcg, current_node_id, link_id, w);
            cstar_rcg_add_edge(rcg, link_id, nbr_id, dist - w);
            // Remove the pre-existing direct current <-> nbr edge that the
            // link node now replaces. Without this, A* sees a stale shortcut
            // that bypasses the logically-split chain.
            cstar_rcg_remove_edge(rcg, current_node_id, nbr_id);

            // Wire cross-lap neighbors for the new link node.
            // Scan all current RCG nodes: connect every Open node on an
            // adjacent lap (lap_id ± 1) within √2·w to the link node.
            // cstar_rcg_add_edge only grows rcg->edges — node pointers stay
            // valid for the entire scan.
            {
                float cross_threshold = sqrtf(2.0f) * w;

                // Re-fetch link_mut: a second cstar_rcg_add_node earlier in
                // this d-loop iteration may have reallocated rcg->nodes.
                link_mut = cstar_rcg_get_node_by_id_mut(rcg, link_id);
                if (link_mut != NULL)
                {
                    int link_lap = link_mut->lap_id;

                    for (int ni = 0; ni < rcg->node_count; ++ni)
                    {
                        cstar_node_t *cand = &rcg->nodes[ni];

                        if (cand->id == link_id)
                        {
                            continue; // self
                        }
                        if (cand->state != CSTAR_NODE_OP)
                        {
                            continue; // already visited
                        }
                        if (cand->is_start_point)
                        {
                            continue; // never expose start as cross-lap target
                        }

                        int lap_diff = cand->lap_id - link_lap;
                        if (lap_diff != -1 && lap_diff != 1)
                        {
                            continue; // not on an adjacent lap
                        }

                        float cdx = cand->pos.x - link_mut->pos.x;
                        float cdy = cand->pos.y - link_mut->pos.y;
                        float cdist = sqrtf(cdx * cdx + cdy * cdy);
                        if (cdist > cross_threshold + 1e-6f)
                        {
                            continue; // out of range
                        }

                        if (lap_diff == -1)
                        {
                            // candidate is on the left lap
                            if (link_mut->neighbors_left_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                            {
                                link_mut->neighbors_left[link_mut->neighbors_left_count++] = cand->id;
                            }
                            if (cand->neighbors_right_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                            {
                                cand->neighbors_right[cand->neighbors_right_count++] = link_id;
                            }
                        }
                        else
                        {
                            // candidate is on the right lap
                            if (link_mut->neighbors_right_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                            {
                                link_mut->neighbors_right[link_mut->neighbors_right_count++] = cand->id;
                            }
                            if (cand->neighbors_left_count < CSTAR_MAX_CROSS_LAP_NEIGHBORS)
                            {
                                cand->neighbors_left[cand->neighbors_left_count++] = link_id;
                            }
                        }

                        cstar_rcg_add_edge(rcg, link_id, cand->id, cdist);
                    }
                }
            }

            link_nodes_created++;
        }
    }

    // --- Close the current node ---
    cstar_node_t *cur_final = cstar_rcg_get_node_by_id_mut(rcg, current_node_id);
    if (cur_final != NULL)
    {
        cur_final->state = CSTAR_NODE_CL;
    }

    return link_nodes_created;
}
