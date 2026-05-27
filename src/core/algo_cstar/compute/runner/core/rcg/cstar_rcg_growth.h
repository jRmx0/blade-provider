#ifndef CSTAR_RCG_GROWTH_H
#define CSTAR_RCG_GROWTH_H

#include <stdbool.h>
#include "../../../../cstar.h"

// -------------------------------------------------------------------------
// RCG Graph Expansion
// -------------------------------------------------------------------------

/**
 * Expands the RCG by connecting frontier-sampled nodes across adjacent laps.
 *
 * For each pair of adjacent laps, connects all node pairs within Euclidean
 * distance √2*w by setting neighbor_left/neighbor_right pointers and adding
 * edges to rcg->edges.
 *
 * Same-lap (vertical) edges are handled separately by
 * cstar_rcg_generate_vertical_lap_edges(). Connectivity and planarity
 * validation is also deferred to that function.
 *
 * Parameters:
 *   rcg - RCG with sampled nodes already added via cstar_rcg_add_node()
 *   env - Environment with pre-generated laps and sampled node_ids per lap
 *
 * Returns:
 *   true  - Cross-lap edges successfully added
 *   false - Invalid inputs
 *
 * Side effects:
 *   - Modifies rcg->nodes (sets neighbors_left/right arrays)
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

// -------------------------------------------------------------------------
// Edge Management
// -------------------------------------------------------------------------

/**
 * Adds an undirected edge between two nodes with the given cost.
 *
 * Both node IDs must exist in the RCG. Does not deduplicate — use
 * cstar_rcg_add_unique_edge if deduplication is required. This function
 * is also used by cstar_waypoint.c when inserting link nodes.
 *
 * Parameters:
 *   rcg    - RCG structure (must not be NULL)
 *   node_a - ID of first node
 *   node_b - ID of second node
 *   cost   - Edge cost (typically Euclidean distance)
 */
void cstar_rcg_add_edge(cstar_rcg_t *rcg,
                        int node_a,
                        int node_b,
                        float cost);

#endif // CSTAR_RCG_GROWTH_H
