#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Memory Management
// -------------------------------------------------------------------------

void cstar_rcg_init(cstar_rcg_t *rcg)
{
    if (rcg == NULL)
    {
        return;
    }

    rcg->nodes = NULL;
    rcg->node_count = 0;
    rcg->node_capacity = 0;
    rcg->next_node_id = 0;

    rcg->edges = NULL;
    rcg->edge_count = 0;
    rcg->edge_capacity = 0;
}

void cstar_rcg_free(cstar_rcg_t *rcg)
{
    if (rcg == NULL)
    {
        return;
    }

    if (rcg->nodes != NULL)
    {
        cvector_free(rcg->nodes);
        rcg->nodes = NULL;
    }
    rcg->node_count = 0;
    rcg->node_capacity = 0;
    rcg->next_node_id = 0;

    if (rcg->edges != NULL)
    {
        cvector_free(rcg->edges);
        rcg->edges = NULL;
    }
    rcg->edge_count = 0;
    rcg->edge_capacity = 0;
}

// -------------------------------------------------------------------------
// Node Management
// -------------------------------------------------------------------------

int cstar_rcg_add_node(cstar_rcg_t *rcg,
                       point_t pos,
                       int lap_id,
                       bool is_end,
                       bool is_start_point)
{
    if (rcg == NULL)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    cstar_node_t new_node = {0};
    new_node.id = rcg->next_node_id;
    new_node.pos = pos;
    new_node.state = CSTAR_NODE_OP;

    new_node.lap_id = lap_id;
    new_node.is_end_node = is_end;
    new_node.is_link_node = false;
    new_node.is_start_point = is_start_point;

    new_node.neighbor_up = CSTAR_NO_NEIGHBOR;
    new_node.neighbor_down = CSTAR_NO_NEIGHBOR;
    new_node.neighbors_left_count = 0;
    new_node.neighbors_right_count = 0;

    cvector_push_back(rcg->nodes, new_node);

    int node_id = new_node.id;
    rcg->next_node_id++;
    rcg->node_count = (int)cvector_size(rcg->nodes);
    rcg->node_capacity = (int)cvector_capacity(rcg->nodes);

    return node_id;
}

int cstar_rcg_index_from_node_id(const cstar_rcg_t *rcg, int node_id)
{
    if (rcg == NULL || rcg->nodes == NULL || node_id == CSTAR_NO_NEIGHBOR)
    {
        return CSTAR_NO_NEIGHBOR;
    }

    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (rcg->nodes[i].id == node_id)
        {
            return i;
        }
    }

    return CSTAR_NO_NEIGHBOR;
}

const cstar_node_t *cstar_rcg_get_node_by_id(const cstar_rcg_t *rcg, int node_id)
{
    int idx = cstar_rcg_index_from_node_id(rcg, node_id);
    if (idx == CSTAR_NO_NEIGHBOR)
    {
        return NULL;
    }

    return &rcg->nodes[idx];
}

cstar_node_t *cstar_rcg_get_node_by_id_mut(cstar_rcg_t *rcg, int node_id)
{
    int idx = cstar_rcg_index_from_node_id(rcg, node_id);
    if (idx == CSTAR_NO_NEIGHBOR)
    {
        return NULL;
    }

    return &rcg->nodes[idx];
}
