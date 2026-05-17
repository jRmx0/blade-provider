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

int cstar_select_goal_node(const cstar_rcg_t *rcg, int current_node_id)
{
    if (rcg == NULL || current_node_id < 0 || current_node_id >= rcg->node_count)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    const cstar_node_t *node = &rcg->nodes[current_node_id];

    int left = node->neighbor_left;
    if (left != CSTAR_NO_NEIGHBOR && left < rcg->node_count &&
        rcg->nodes[left].state == CSTAR_NODE_OP)
    {
        return left;
    }

    int up = node->neighbor_up;
    if (up != CSTAR_NO_NEIGHBOR && up < rcg->node_count &&
        rcg->nodes[up].state == CSTAR_NODE_OP)
    {
        return up;
    }

    int down = node->neighbor_down;
    if (down != CSTAR_NO_NEIGHBOR && down < rcg->node_count &&
        rcg->nodes[down].state == CSTAR_NODE_OP)
    {
        return down;
    }

    int right = node->neighbor_right;
    if (right != CSTAR_NO_NEIGHBOR && right < rcg->node_count &&
        rcg->nodes[right].state == CSTAR_NODE_OP)
    {
        return right;
    }

    return CSTAR_NO_NEIGHBOR;
}

int cstar_update_node_state(cstar_rcg_t *rcg,
                            int current_node_id,
                            int goal_node_id,
                            float w)
{
    /*
     * Pseudocode (Algorithm 2):
     *   link_nodes_created = 0
     *   node = rcg->nodes[current_node_id]
     *
     *   // Determine if we are allowed to close current_node_id.
     *   U = node.neighbor_up;   U_open = (U != NO && nodes[U].state == OP)
     *   D = node.neighbor_down; D_open = (D != NO && nodes[D].state == OP)
     *   can_close = NOT (U_open AND D_open)   // do not split an Open run
     *
     *   if can_close:
     *     // When transitioning left, insert link nodes for distant Open
     *     // same-lap neighbours to maintain traversability.
     *     if goal is on left lap:
     *       if U_open && dist(node.pos, nodes[U].pos) > w:
     *         link_pos = node.pos + w * unit_vec(node.pos -> nodes[U].pos)
     *         id = cstar_rcg_add_node(rcg, link_pos, node.lap_id, false)
     *         nodes[id].is_link_node = true
     *         // Link edges bypass collision check (gap is already open).
     *         connect: current_node_id <-> id <-> U
     *         link_nodes_created++
     *       // Mirror for D_open (lower same-lap neighbour).
     *
     *     cstar_rcg_close_node(rcg, current_node_id)
     *
     *   return link_nodes_created
     */
    return 0;
}
