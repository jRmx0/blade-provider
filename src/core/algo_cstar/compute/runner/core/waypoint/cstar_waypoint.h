#ifndef CSTAR_WAYPOINT_H
#define CSTAR_WAYPOINT_H

#include "../../../../cstar.h"

// -------------------------------------------------------------------------
// Goal node selection  (Algorithm 1)
// -------------------------------------------------------------------------

/**
 * Selects the next goal node from the RCG using the priority order
 *   left → up → down → right  (Algorithm 1).
 *
 * Returns the index of the selected Open neighbour, or CSTAR_NO_NEIGHBOR
 * when all four directional neighbours are Closed / absent (dead-end).
 * The caller is responsible for invoking cstar_escape_dead_end() in that case.
 *
 * When multiple Open neighbours exist on the left or right adjacent lap,
 * one is chosen at random.
 */
int cstar_select_goal_node(const cstar_rcg_t *rcg, int current_node_id);

// -------------------------------------------------------------------------
// State update  (Algorithm 2)
// -------------------------------------------------------------------------

/**
 * Updates the state of current_node_id after goal_node_id has been selected
 * (Algorithm 2):
 *
 *   - Closes current_node_id unless both same-lap neighbours (up and down)
 *     are Open (to avoid splitting an Open run on a lap).
 *   - If the goal was selected on the left lap and the current node is being
 *     closed, creates a link node above and/or below at distance w for any
 *     Open same-lap neighbour that is farther than w.
 *
 * w   - sampling resolution (metres)
 *
 * Returns the number of link nodes created (0, 1, or 2).
 */
int cstar_update_node_state(cstar_rcg_t *rcg,
                            int current_node_id,
                            int goal_node_id,
                            float w);

#endif // CSTAR_WAYPOINT_H
