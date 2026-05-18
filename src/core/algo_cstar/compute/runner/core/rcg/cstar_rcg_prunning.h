
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

#endif // CSTAR_RCG_PRUNNING_H
