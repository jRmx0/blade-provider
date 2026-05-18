#ifndef CSTAR_DEBUG_H
#define CSTAR_DEBUG_H

#include "../../../cstar.h"
#include "../core/rcg/cstar_rcg.h"

typedef struct
{
    cstar_debug_point_list_t rcg_link_nodes;
    cstar_debug_point_list_t rcg_end_nodes;
    cstar_debug_segment_list_t rcg_edges;
    cstar_debug_segment_list_t lap_list;
    cstar_debug_point_list_t frontier_sample_list;
    cstar_debug_point_list_t retreat_node_list;
    cstar_debug_point_list_t coverage_hole_list;
} cstar_debug_t;

bool cstar_debug_init(cstar_debug_t *debug_state);
void cstar_debug_dispose(cstar_debug_t *debug_state);

bool cstar_debug_finalize_layers(cstar_debug_t *debug_state,
                                 cstar_debug_layers_t *out_layers);

bool cstar_debug_export_rcg_nodes(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg);

/**
 * Same as cstar_debug_export_rcg_nodes but only exports nodes whose array
 * index is >= from_index. Use after mid-loop RCG insertions to append only
 * the newly-added nodes without duplicating pre-existing ones.
 */
bool cstar_debug_export_rcg_nodes_from_index(cstar_debug_t *debug_state,
                                             const cstar_rcg_t *rcg,
                                             int from_index);

/**
 * Appends all link nodes (is_link_node == true) created during the coverage
 * loop to the rcg_link_nodes debug list.
 *
 * Call this after the main coverage loop completes, before
 * cstar_debug_finalize_layers, so that dynamically-created link nodes are
 * captured in the rcgLinkNodeList debug layer (id=10).
 */
bool cstar_debug_export_link_nodes(cstar_debug_t *debug_state,
                                   const cstar_rcg_t *rcg);

bool cstar_debug_export_rcg_edges(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg);

bool cstar_debug_export_laps(cstar_debug_t *debug_state,
                             const cstar_environment_t *env);

/**
 * Accumulates retreat nodes into the retreatNodeList debug layer (id=16).
 *
 * Call this after each cstar_retreat_update() call in the main coverage loop.
 * Deduplicates by node ID so nodes that persist across multiple iterations
 * appear only once. Produces a union of all nodes ever in the retreat set.
 *
 * retreat_nodes holds node IDs (int); positions are resolved via the RCG.
 */
bool cstar_debug_accumulate_retreat_nodes(cstar_debug_t *debug_state,
                                          cvector_vector_type(int) retreat_nodes,
                                          const cstar_rcg_t *rcg);

/**
 * Accumulates coverage hole nodes into the coverageHoleList debug layer (id=17).
 *
 * Appends each node in node_ids[0..node_count-1] as a point entry so that
 * detected hole nodes are visible in the debug output regardless of TSP success.
 * Call once per detected hole before executing the TSP trajectory.
 */
bool cstar_debug_accumulate_coverage_holes(cstar_debug_t *debug_state,
                                           const int *node_ids,
                                           int node_count,
                                           const cstar_rcg_t *rcg);

#endif // CSTAR_DEBUG_H
