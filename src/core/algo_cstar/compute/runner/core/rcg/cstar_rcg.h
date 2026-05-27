#ifndef CSTAR_RCG_H
#define CSTAR_RCG_H

#include <stdbool.h>
#include "../../../../cstar.h"

// -------------------------------------------------------------------------
// RCG Core Memory Management
// -------------------------------------------------------------------------

/**
 * Initializes an empty Rapidly Covering Graph.
 *
 * Sets all node and edge arrays to NULL/0. This must be called before
 * any other RCG operations.
 *
 * Parameters:
 *   rcg - RCG structure to initialize (must not be NULL)
 */
void cstar_rcg_init(cstar_rcg_t *rcg);

/**
 * Frees all memory allocated for the RCG.
 *
 * Deallocates node and edge arrays and resets the RCG structure to an
 * empty state. Safe to call multiple times.
 *
 * Parameters:
 *   rcg - RCG structure to free (must not be NULL)
 */
void cstar_rcg_free(cstar_rcg_t *rcg);

// -------------------------------------------------------------------------
// RCG Node Management
// -------------------------------------------------------------------------

/**
 * Adds a new node to the RCG and returns its stable node ID.
 *
 * Allocates a new node in the rcg->nodes array, initializes its position
 * and lap association, and returns its ID. The node is initially unvisited
 * (state = CSTAR_NODE_OP) with all neighbors set to CSTAR_NO_NEIGHBOR.
 *
 * Parameters:
 *   rcg     - RCG structure (must not be NULL)
 *   pos     - Node position in 2D space
 *   lap_id  - Index of the lap this node belongs to
 *   is_top_end_node    - Whether this node touches an obstacle or boundary in the up (+y) direction
 *   is_bottom_end_node - Whether this node touches an obstacle or boundary in the down (-y) direction
 *   is_top_and_bottom_end_node - Whether this node touches a border from top AND bottom
 *
 * Returns:
 *   Stable node ID on success
 *   CSTAR_NO_NEIGHBOR (-1) on allocation failure
 */
int cstar_rcg_add_node(cstar_rcg_t *rcg,
                       point_t pos,
                       int lap_id,
                       bool is_top_end_node,
                       bool is_bottom_end_node,
                       bool is_top_and_bottom_end_node,
                       bool is_start_point);

/**
 * Resolves a stable node ID to its current index in rcg->nodes.
 *
 * Returns CSTAR_NO_NEIGHBOR when the node ID does not exist.
 */
int cstar_rcg_index_from_node_id(const cstar_rcg_t *rcg, int node_id);

/**
 * Resolves a stable node ID to its current index in a raw node array.
 *
 * Allows scanning a temporary array that is not yet owned by an RCG.
 * Returns CSTAR_NO_NEIGHBOR when the node ID does not exist.
 */
int cstar_rcg_index_from_node_id_in_array(const cstar_node_t *nodes,
                                          int node_count,
                                          int node_id);

/**
 * Returns an immutable node pointer by stable node ID.
 *
 * Returns NULL when node ID does not exist.
 */
const cstar_node_t *cstar_rcg_get_node_by_id(const cstar_rcg_t *rcg, int node_id);

/**
 * Returns a mutable node pointer by stable node ID.
 *
 * Returns NULL when node ID does not exist.
 */
cstar_node_t *cstar_rcg_get_node_by_id_mut(cstar_rcg_t *rcg, int node_id);

// -------------------------------------------------------------------------
// RCG Edge Management
// -------------------------------------------------------------------------

/**
 * Removes the first undirected edge matching (node_a, node_b) or
 * (node_b, node_a) from the RCG edge list using a swap-remove.
 *
 * Uses rcg->edge_count as the authoritative logical size; the cvector
 * backing buffer stays allocated. Only the first matching edge is removed.
 * No-op if no matching edge is found.
 *
 * Parameters:
 *   rcg    - RCG structure (must not be NULL)
 *   node_a - ID of one endpoint
 *   node_b - ID of other endpoint
 */
void cstar_rcg_remove_edge(cstar_rcg_t *rcg, int node_a, int node_b);

#endif // CSTAR_RCG_H
