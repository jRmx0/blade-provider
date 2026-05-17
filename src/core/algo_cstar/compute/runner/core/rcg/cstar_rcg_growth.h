#ifndef CSTAR_RCG_GROWTH_H
#define CSTAR_RCG_GROWTH_H

#include "../../../../internal.h"
#include "../sampling/cstar_sampling.h"

// -------------------------------------------------------------------------
// Expansion
// -------------------------------------------------------------------------

/**
 * Expands the RCG into the sampling front by:
 *   i.  Adding frontier samples as new nodes.
 *   ii. Connecting each new node to its adjacent same-lap nodes.
 *   iii.Connecting each new node to nodes within sqrt(2)*w on adjacent laps.
 *
 * Every candidate edge is checked for collision-freeness before being added.
 * Existing nodes are not moved or removed by this step.
 */
void cstar_rcg_expand(cstar_rcg_t *rcg,
                      const cstar_sampling_front_t *front,
                      float w,
                      const cstar_environment_t *env);

// -------------------------------------------------------------------------
// Pruning
// -------------------------------------------------------------------------

/**
 * Prunes inessential nodes and edges from the expanded RCG to restore the
 * sparse-graph invariant (Definitions III.8 and III.9).
 *
 * boundary_node_ids - indices of previously-existing nodes that border the
 *                     sampling-front boundary ∂F_i; these are re-evaluated
 *                     because the expansion may have made them inessential.
 * boundary_count    - length of boundary_node_ids
 */
void cstar_rcg_prune(cstar_rcg_t *rcg,
                     const int *boundary_node_ids,
                     int boundary_count,
                     float w,
                     const cstar_environment_t *env);

// -------------------------------------------------------------------------
// Essentialness predicates
// -------------------------------------------------------------------------

/**
 * Returns true if node_id is essential per Definition III.8:
 *   1. Adjacent to the unknown area, OR
 *   2. An end node of its lap, OR
 *   3. A non-end node connected to an end node of an adjacent lap, and the
 *      connection is the sole or closest-to-obstacle link for that end node.
 */
bool cstar_node_is_essential(const cstar_rcg_t *rcg, int node_id,
                             float w, const cstar_environment_t *env);

/**
 * Returns true if the edge (node_a, node_b) is essential per Definition III.9.
 * Both nodes must already be essential for an edge to qualify.
 */
bool cstar_edge_is_essential(const cstar_rcg_t *rcg, int node_a, int node_b,
                             float w, const cstar_environment_t *env);

#endif // CSTAR_RCG_GROWTH_H
