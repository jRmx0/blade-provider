
#include "cstar_rcg_prunning.h"
#include <stdlib.h>
#include <stdbool.h>
#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// Prune the RCG, leaving only end nodes and edges between them.

void cstar_rcg_prune_to_end_nodes(cstar_rcg_t *rcg)
{
    if (!rcg || !rcg->nodes)
        return;

    // Step 1: Mark non-end nodes for removal, but always keep the node at start_point
    int *keep_node = (int *)calloc(rcg->node_count, sizeof(int));
    int new_count = 0;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        int keep = 0;
        if (rcg->nodes[i].is_end_node)
            keep = 1;
        // Also keep node marked as start point node
        if (rcg->nodes[i].is_start_point)
            keep = 1;
        if (keep)
        {
            keep_node[i] = 1;
            ++new_count;
        }
    }

    // Step 2: Build new node array with only kept nodes
    cstar_node_t *new_nodes = NULL;
    int *old_to_new = (int *)malloc(rcg->node_count * sizeof(int));
    int idx = 0;
    for (int i = 0; i < rcg->node_count; ++i)
    {
        if (keep_node[i])
        {
            cvector_push_back(new_nodes, rcg->nodes[i]);
            old_to_new[i] = idx++;
        }
        else
        {
            old_to_new[i] = -1;
        }
    }

    // Step 3: Remove edges not between two kept nodes
    cstar_edge_t *new_edges = NULL;
    for (int i = 0; i < rcg->edge_count; ++i)
    {
        int a = rcg->edges[i].node_a;
        int b = rcg->edges[i].node_b;
        if (a >= 0 && b >= 0 && keep_node[a] && keep_node[b])
        {
            // Remap node indices
            cstar_edge_t e = rcg->edges[i];
            e.node_a = old_to_new[a];
            e.node_b = old_to_new[b];
            cvector_push_back(new_edges, e);
        }
    }

    // Step 4: Replace old arrays
    cvector_free(rcg->nodes);
    rcg->nodes = new_nodes;
    rcg->node_count = (int)cvector_size(new_nodes);
    rcg->node_capacity = (int)cvector_capacity(new_nodes);

    cvector_free(rcg->edges);
    rcg->edges = new_edges;
    rcg->edge_count = (int)cvector_size(new_edges);
    rcg->edge_capacity = (int)cvector_capacity(new_edges);

    free(keep_node);
    free(old_to_new);
}
