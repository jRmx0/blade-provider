/**
 * cstar.h
 *
 * Public API and shared type definitions for the C* algorithm module.
 * Contains C*-specific types, constants, and function signatures used across
 * the module, including output structures for coverage path planning results.
 *
 * Fundamental geometry and environment types (point_t, polygon_t,
 * input_environment_t, etc.) are in src/core/core_types.h, included below.
 */

#ifndef CSTAR_H
#define CSTAR_H

#include <stdint.h>
#include <stdbool.h>
#include "../core_types.h"

// -------------------------------------------------------------------------
// C* Environment Type (algo_cstar specific)
// -------------------------------------------------------------------------

/**
 * Environment structure for C* coverage path planning algorithm.
 * Built from input_environment_t but contains operational geometry
 * (shrunken boundary, expanded obstacles) ready for algorithm processing.
 */
typedef struct
{
    uint32_t id;
    float path_width;
    float sensor_range;
    uint32_t frontier_spacing_multiplier;
    bool track_memory_usage;
    bool headland;

    point_t start_point;
    point_t end_point;

    polygon_t operationalBoundary;   // Shrunken real boundary
    polygon_t *operationalObstacles; // Expanded real obstacles
    uint32_t obstacle_count;
} cstar_environment_t;

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
// Coverage Path Planning Output Structures
// -------------------------------------------------------------------------

/**
 * A single waypoint in a path segment.
 * Corresponds to { "id": <int>, "point": { "x": <float>, "y": <float> } }
 */
typedef struct
{
    int id;
    point_t point;
} cstar_path_point_t;

/**
 * A path segment (e.g., coverage line, transit line).
 * Represents one path unit with a type and ordered waypoints.
 * Corresponds to { "id": <int>, "type": "<string>", "path": [...] }
 */
typedef struct
{
    int id;
    char *type; // "coverage", "coverageTransit", "retreatTransit", etc. (owned by struct)
    cstar_path_point_t *path;
    int path_count;
} cstar_segment_t;

/**
 * A collection of segments grouped by type/category.
 * Used to organize coverage, coverage_transit, retreat_transit, etc.
 */
typedef struct
{
    cstar_segment_t *segments;
    int segment_count;
    int segment_capacity;
} cstar_path_collection_t;

typedef enum
{
    CSTAR_DEBUG_LIST_POINTS = 0,
    CSTAR_DEBUG_LIST_SEGMENTS = 1,
    CSTAR_DEBUG_LIST_POLYGONS = 2,
} cstar_debug_list_type_t;

typedef struct
{
    int id;
    point_t point;
} cstar_debug_point_entry_t;

typedef struct
{
    int id;
    const char *type;
    point_t *path;
    int path_count;
} cstar_debug_segment_entry_t;

typedef struct
{
    int id;
    point_t *vertices;
    int vertex_count;
} cstar_debug_polygon_entry_t;

typedef struct
{
    cstar_debug_point_entry_t *items;
    int count;
    int capacity;
} cstar_debug_point_list_t;

typedef struct
{
    cstar_debug_segment_entry_t *items;
    int count;
    int capacity;
} cstar_debug_segment_list_t;

typedef struct
{
    cstar_debug_polygon_entry_t *items;
    int count;
    int capacity;
} cstar_debug_polygon_list_t;

typedef struct
{
    int id;
    const char *source;
    cstar_debug_list_type_t list_type;
    union
    {
        cstar_debug_point_list_t points;
        cstar_debug_segment_list_t segments;
        cstar_debug_polygon_list_t polygons;
    } list;
} cstar_debug_layer_t;

typedef struct
{
    cstar_debug_layer_t *layers;
    int layer_count;
    int layer_capacity;
} cstar_debug_layers_t;

/**
 * Complete C* coverage path planning result.
 * Contains all segment arrays, debug information, and algorithm state.
 */
typedef struct
{
    // Segments array: all segments in order of generation
    cstar_segment_t *all_segments;
    int segment_count;
    int segment_capacity;

    // Categorized segment collections for wire output
    cstar_path_collection_t coverage;
    cstar_path_collection_t coverage_transit;
    cstar_path_collection_t retreat_transit;
    cstar_path_collection_t hole_coverage;
    cstar_path_collection_t hole_transit;

    // Optional debug information emitted by compute as typed structs.
    cstar_debug_layers_t debug_layers;
} cstar_coverage_path_result_t;

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

char *cstar_get_metadata_json(void);
char *cstar_compute(const char *input_environment_json);

// -------------------------------------------------------------------------
// Internal API (used by sub-modules during computation)
// -------------------------------------------------------------------------

char *cstar_build_metadata_json(void);
cstar_coverage_path_result_t *cstar_run_compute(cstar_environment_t *environment);

#endif // CSTAR_H