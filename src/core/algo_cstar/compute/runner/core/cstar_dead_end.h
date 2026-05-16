#ifndef CSTAR_DEAD_END_H
#define CSTAR_DEAD_END_H

#include "../../../internal.h"
#include "../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Dead-end detection
// -------------------------------------------------------------------------

/**
 * Returns true if node_id is a dead-end (Definition III.12):
 * all four directional neighbours are Closed or absent.
 */
bool cstar_is_dead_end(const cstar_rcg_t *rcg, int node_id);

// -------------------------------------------------------------------------
// Retreat node management
// -------------------------------------------------------------------------

/**
 * Synchronises the retreat-node set for the current robot position:
 *   - Adds any Open node within sqrt(2)*w of robot_pos that is not already
 *     in the set (Definition III.13).
 *   - Removes any node that has become Closed.
 *
 * Must be called once per iteration, after the RCG state is updated.
 */
void cstar_retreat_update(cvector_vector_type(int) * retreat_nodes,
                          const cstar_rcg_t *rcg,
                          point_t robot_pos,
                          float w);

// -------------------------------------------------------------------------
// Dead-end escape
// -------------------------------------------------------------------------

/**
 * Selects the nearest retreat node via A* and returns its index.
 * Returns CSTAR_NO_NEIGHBOR if retreat_nodes is empty (coverage is complete).
 *
 * path_out - if non-NULL, receives a cvector(point_t) of waypoints describing
 *            the shortest path from current_node_id to the chosen retreat node.
 *            The caller is responsible for calling cvector_free() on it.
 */
int cstar_escape_dead_end(const cstar_rcg_t *rcg,
                          int current_node_id,
                          const cvector_vector_type(int) retreat_nodes,
                          cvector_vector_type(point_t) * path_out);

#endif // CSTAR_DEAD_END_H
