#ifndef CSTAR_RCG_H
#define CSTAR_RCG_H

#include "../../../internal.h"

/**
 * Initialises an empty RCG. Must be paired with cstar_rcg_free().
 */
void cstar_rcg_init(cstar_rcg_t *rcg);

/**
 * Frees all memory owned by the RCG.
 */
void cstar_rcg_free(cstar_rcg_t *rcg);

/**
 * Adds a node to the RCG. Returns the new node's index.
 */
int cstar_rcg_add_node(cstar_rcg_t *rcg, point_t pos, int lap_id, bool is_end_node);

/**
 * Adds a directed edge between node_a and node_b after verifying the segment
 * lies entirely in obstacle-free space.
 * Returns true if the edge was added, false if it would cross an obstacle.
 */
bool cstar_rcg_add_edge(cstar_rcg_t *rcg, int node_a, int node_b,
                        const input_environment_t *env);

/**
 * Removes a node by index. Clears all neighbour references pointing to it.
 * Does not merge any edges; call cstar_rcg_merge_lap_edge() first if needed.
 */
void cstar_rcg_remove_node(cstar_rcg_t *rcg, int node_id);

/**
 * Removes the edge between node_a and node_b, if it exists.
 */
void cstar_rcg_remove_edge(cstar_rcg_t *rcg, int node_a, int node_b);

/**
 * Merges the two same-lap edges adjacent to node_id into a single edge
 * spanning its up- and down-neighbours. Used when pruning an inessential node
 * that sits in the middle of a lap.
 */
void cstar_rcg_merge_lap_edge(cstar_rcg_t *rcg, int node_id);

/**
 * Returns true if the straight segment from a to b lies entirely in
 * obstacle-free space (no polygon edge crossings, not inside any obstacle).
 */
bool cstar_rcg_edge_is_collision_free(point_t a, point_t b,
                                      const input_environment_t *env);

/**
 * Marks node node_id as CSTAR_NODE_CL (visited).
 */
void cstar_rcg_close_node(cstar_rcg_t *rcg, int node_id);

#endif // CSTAR_RCG_H
