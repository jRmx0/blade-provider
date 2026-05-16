/**
 * internal.h
 *
 * Private shared definitions for the C* algorithm module.
 * Contains C*-specific internal structs, types, constants, and helper
 * function signatures used across metadata/ and compute/ files.
 *
 * Fundamental geometry and environment types (point_t, polygon_t,
 * input_environment_t, etc.) are in src/core/core_types.h, included below.
 */

#ifndef CSTAR_INTERNAL_H
#define CSTAR_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../core_types.h"

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

/** Sentinel value for absent directional neighbors. */
#define CSTAR_NO_NEIGHBOR -1

// -------------------------------------------------------------------------
// RCG node
// -------------------------------------------------------------------------

/** State of an RCG node: Op = unvisited, Cl = visited. */
typedef enum
{
    CSTAR_NODE_OP = 0,
    CSTAR_NODE_CL = 1
} cstar_node_state_t;

/**
 * A node in the Rapidly Covering Graph (RCG).
 *
 * Directional neighbors are expressed as indices into cstar_rcg_t.nodes.
 * CSTAR_NO_NEIGHBOR (-1) means no neighbor in that direction.
 *
 * Directions are defined in a fixed coordinate frame whose vertical axis
 * is parallel to the laps (i.e., laps run vertically, back-and-forth
 * motion is horizontal left→right).
 */
typedef struct
{
    int id;
    point_t pos;
    cstar_node_state_t state;

    int lap_id;        // index of the lap this node belongs to
    bool is_end_node;  // true if this node touches an obstacle or boundary
    bool is_link_node; // true if created by the state-update step

    int neighbor_up;    // same lap, increasing-y direction
    int neighbor_down;  // same lap, decreasing-y direction
    int neighbor_left;  // left adjacent lap
    int neighbor_right; // right adjacent lap
} cstar_node_t;

// -------------------------------------------------------------------------
// RCG edge
// -------------------------------------------------------------------------

typedef struct
{
    int node_a;
    int node_b;
    float cost; // Euclidean distance
} cstar_edge_t;

// -------------------------------------------------------------------------
// Rapidly Covering Graph (RCG)
// -------------------------------------------------------------------------

typedef struct
{
    cstar_node_t *nodes;
    int node_count;
    int node_capacity;

    cstar_edge_t *edges;
    int edge_count;
    int edge_capacity;
} cstar_rcg_t;

// -------------------------------------------------------------------------
// Lap
// -------------------------------------------------------------------------

/**
 * A virtual straight line segment in the coverage space.
 * Laps are parallel to each other, spaced w apart, and run along the
 * back-and-forth axis. Both ends terminate at an obstacle or boundary.
 */
typedef struct
{
    int id;
    float x;       // position along the cross-lap axis
    int *node_ids; // ordered node indices on this lap (by position along lap)
    int node_count;
    int node_capacity;
} cstar_lap_t;

// -------------------------------------------------------------------------
// API (implemented in metadata/ and compute/)
// -------------------------------------------------------------------------

char *cstar_build_metadata_json(void);
char *cstar_run_compute(const char *input_environment_json);

#endif // CSTAR_INTERNAL_H
