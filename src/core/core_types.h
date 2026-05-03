/**
 * core_types.h
 *
 * Fundamental geometry and environment types shared across all algorithms
 * in src/core/. Must not depend on any algorithm-specific types.
 *
 * Included by: algo_bcd/internal.h, src/core/common/ modules
 */

#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "../../dependencies/cvector/cvector.h"

typedef struct
{
    float x;
    float y;
} point_t;

typedef struct
{
    point_t begin;
    point_t end;
} polygon_edge_t;

typedef enum
{
    POLYGON_WINDING_UNKNOWN = 0,
    POLYGON_WINDING_CW = 1, // Boundary winding type
    POLYGON_WINDING_CCW = 2 // Obstacle winding type
} polygon_winding_t;

typedef enum
{
    BOUNDARY,
    OBSTACLE
} polygon_type_t;

typedef struct
{
    polygon_winding_t winding;

    point_t *vertices;
    uint32_t vertex_count;

    polygon_edge_t *edges;
    uint32_t edge_count;
} polygon_t;

typedef struct
{
    uint32_t id;
    float path_width;
    float path_overlap;
    float headland_coverage_offset;
    float bounce_offset;
    float target_coverage;
    float target_distance;
    uint32_t max_iterations;
    bool track_memory_usage;
    bool headland;
    float starting_angle; // Initial travel direction in degrees [0, 360]; -1 = pick randomly.

    point_t start_point;
    point_t end_point;

    polygon_t boundary;

    polygon_t *obstacles;
    uint32_t obstacle_count;
} input_environment_t;

// polygon_t lifecycle
void free_polygon(polygon_t *polygon);

#endif // CORE_TYPES_H
