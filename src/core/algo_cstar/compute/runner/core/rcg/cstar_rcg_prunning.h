
#ifndef CSTAR_RCG_PRUNNING_H
#define CSTAR_RCG_PRUNNING_H

#include "../../../../cstar.h"

/**
 * Prunes the RCG, removing non-essential nodes and their incident edges.
 *
 * Keeps a node if it satisfies any of the following:
 *   - Is a top or bottom end node (touches obstacle or boundary)
 *   - Is the start point
 *   - Is the sole cross-lap bridge to an end node on an adjacent lap
 *   - Is the closest-to-boundary neighbor of an end node on an adjacent lap,
 *     when all other neighbors from that lap are non-end nodes
 *
 * After pruning, rebuilds neighbor pointer adjacency from the surviving edge set.
 *
 * @param rcg Pointer to the RCG graph to prune (in-place).
 */
void cstar_rcg_prune_non_essential_nodes(cstar_rcg_t *rcg);

/**
 * Full graph update: flushes all edges and re-derives cross-lap and same-lap
 * edges from scratch for the current node set, then syncs all in-node neighbor
 * pointers.
 *
 * Call after cstar_rcg_prune_non_essential_nodes() to rebuild a fully consistent
 * graph from the surviving node set.  Used at both the initial setup site and
 * the post-obstacle site.
 *
 * @param rcg Pointer to the pruned RCG graph.
 * @param env Environment with pre-generated laps.
 */
void cstar_rcg_full_graph_update(cstar_rcg_t *rcg, const cstar_environment_t *env);

#endif // CSTAR_RCG_PRUNNING_H
