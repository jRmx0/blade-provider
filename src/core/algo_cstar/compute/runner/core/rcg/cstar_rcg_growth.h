#ifndef CSTAR_RCG_GROWTH_H
#define CSTAR_RCG_GROWTH_H

#include <stdbool.h>
#include "../../../../cstar.h"

// -------------------------------------------------------------------------
// RCG Graph Expansion
// -------------------------------------------------------------------------

/**
 * Expands the RCG by connecting frontier-sampled nodes into a planar graph.
 *
 * Performs three stages of graph growth:
 *
 * 1. **Same-lap vertical connectivity**: For each lap, connects adjacent nodes
 *    (ordered along the lap's y-axis) by setting neighbor_up/neighbor_down
 *    pointers.
 *
 * 2. **Cross-lap horizontal connectivity**: For each pair of adjacent laps,
 *    connects nodes within Euclidean distance √2*w by setting
 *    neighbor_left/neighbor_right pointers and adding edges.
 *
 * 3. **Validation**: Verifies graph connectivity (all nodes reachable from
 *    node[0]) and planarity (edges ≤ 3*nodes - 6). Returns false if
 *    validation fails.
 *
 * Edge costs are stored as Euclidean distance for waypoint selection.
 *
 * Parameters:
 *   rcg - RCG with sampled nodes already added via cstar_rcg_add_node()
 *   env - Environment with pre-generated laps and sampled node_ids per lap
 *
 * Returns:
 *   true  - Graph successfully expanded and validated
 *   false - Allocation error, validation failure, or invalid inputs
 *
 * Side effects:
 *   - Modifies rcg->nodes (sets neighbor pointers)
 *   - Populates rcg->edges with cross-lap connections
 *   - No changes to env
 */
bool cstar_rcg_expand_graph(cstar_rcg_t *rcg,
                            const cstar_environment_t *env);

// -------------------------------------------------------------------------
// Post-Pruning Edge Generation
// -------------------------------------------------------------------------

/**
 * Generates vertical edges within each lap for the pruned RCG.
 *
 * Called after cstar_rcg_prune_non_essential_nodes(). For each lap, connects
 * surviving end nodes top-to-bottom with one downward edge per node.
 * Also connects the start point to its nearest neighbour. Validates
 * connectivity and planarity after all edges are added.
 *
 * Parameters:
 *   rcg - Pruned RCG (must not be NULL)
 *   env - Environment with pre-generated laps (must not be NULL)
 */
void cstar_rcg_generate_vertical_lap_edges(cstar_rcg_t *rcg,
                                           const cstar_environment_t *env);

#endif // CSTAR_RCG_GROWTH_H
