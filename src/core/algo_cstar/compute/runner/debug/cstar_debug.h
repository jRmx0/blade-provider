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
    cstar_debug_polygon_list_t coverage_hole_list;
} cstar_debug_t;

bool cstar_debug_init(cstar_debug_t *debug_state);
void cstar_debug_dispose(cstar_debug_t *debug_state);

bool cstar_debug_finalize_layers(cstar_debug_t *debug_state,
                                 cstar_debug_layers_t *out_layers);

bool cstar_debug_export_rcg_nodes(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg);

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

#endif // CSTAR_DEBUG_H
