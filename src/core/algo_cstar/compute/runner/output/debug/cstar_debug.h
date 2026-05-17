#ifndef CSTAR_DEBUG_H
#define CSTAR_DEBUG_H

#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cJSON/cJSON.h"
#include "../../core/rcg/cstar_rcg.h"
#include "../../core/sampling/cstar_sampling.h"

typedef struct
{
    cJSON *rcg_link_nodes;
    cJSON *rcg_end_nodes;
    cJSON *rcg_edges;
    cJSON *lap_list;
    cJSON *sampling_front_list;
    cJSON *frontier_sample_list;
    cJSON *retreat_node_list;
    cJSON *coverage_hole_list;
    cJSON *debug;
    cJSON *layers;
} cstar_debug_t;

bool cstar_debug_init(cstar_debug_t *debug_state, cJSON *root);
void cstar_debug_dispose(cstar_debug_t *debug_state);

bool cstar_debug_export_rcg_nodes(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg);

bool cstar_debug_export_rcg_edges(cstar_debug_t *debug_state,
                                  const cstar_rcg_t *rcg);

bool cstar_debug_export_laps(cstar_debug_t *debug_state,
                             const cstar_sampling_front_t *front,
                             const cstar_environment_t *env);

bool cstar_debug_export_sampling_front_polygon(cstar_debug_t *debug_state,
                                               const cstar_environment_t *env);

bool cstar_debug_attach_layers(cstar_debug_t *debug_state, cJSON *root);

#endif // CSTAR_DEBUG_H
