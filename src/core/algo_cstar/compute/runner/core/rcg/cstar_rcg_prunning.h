
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

/**
 * Generate vertical same-lap edges between surviving end nodes after pruning.
 * For each lap, connects adjacent surviving nodes top-to-bottom using downward
 * edges only, skipping is_start_point and is_top_and_bottom_end_node nodes.
 *
 * Must be called after cstar_rcg_prune_to_end_nodes.
 *
 * @param rcg Pointer to the pruned RCG graph.
 * @param env Environment containing the lap structure (env->laps).
 */
void cstar_rcg_generate_vertical_lap_edges(cstar_rcg_t *rcg, const cstar_environment_t *env);

#endif // CSTAR_RCG_PRUNNING_H
