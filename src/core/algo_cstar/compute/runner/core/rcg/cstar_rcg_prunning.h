
#ifndef CSTAR_RCG_PRUNNING_H
#define CSTAR_RCG_PRUNNING_H

#include "../../../../cstar.h"

/**
 * Prune the RCG, leaving only end nodes and edges between them.
 * Removes all non-end nodes and their incident edges.
 *
 * @param rcg Pointer to the RCG graph to prune (in-place).
 */
void cstar_rcg_prune_to_end_nodes(cstar_rcg_t *rcg);

#endif // CSTAR_RCG_PRUNNING_H
