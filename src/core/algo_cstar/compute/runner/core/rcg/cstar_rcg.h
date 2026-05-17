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
 * Adds a new node to the RCG and returns its node ID.
 *
 * Allocates a new node in the rcg->nodes array, initializes its position
 * and lap association, and returns its ID. The node is initially unvisited
 * (state = CSTAR_NODE_OP) with all neighbors set to CSTAR_NO_NEIGHBOR.
 *
 * Parameters:
 *   rcg     - RCG structure (must not be NULL)
 *   pos     - Node position in 2D space
 *   lap_id  - Index of the lap this node belongs to
 *   is_end  - Whether this node touches an obstacle or boundary
 *
 * Returns:
 *   Node ID (0-based index into rcg->nodes) on success
 *   CSTAR_NO_NEIGHBOR (-1) on allocation failure
 */
int cstar_rcg_add_node(cstar_rcg_t *rcg,
                       point_t pos,
                       int lap_id,
                       bool is_end,
                       bool is_start_point);

#endif // CSTAR_RCG_H
