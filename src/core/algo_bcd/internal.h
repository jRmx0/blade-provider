/**
 * internal.h
 *
 * Private shared definitions for the BCD algorithm module.
 * Contains internal structs, types, constants, and helper function
 * signatures used across metadata/bcd_metadata.c and compute/ files.
 * Must not be included outside of algo_bcd/.
 *
 * Included by: metadata/bcd_metadata.c, bcd_compute.c, step files
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../../../dependencies/cvector/cvector.h"

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
    bool track_memory_usage;
    bool headland;

    polygon_t boundary;

    polygon_t *obstacles;
    uint32_t obstacle_count;
} input_environment_t;

// Headland types

typedef struct
{
    cvector_vector_type(point_t) path;
    cvector_vector_type(point_t) nav; // transit to next section (or to first coverage point)
    int source_index;                 // -1 = zone boundary, 0+ = obstacle index
} headland_section_t;

typedef struct
{
    cvector_vector_type(headland_section_t) sections;
    polygon_t shrunken_zone;
    polygon_t *expanded_obstacles;
    uint32_t expanded_obstacle_count;
} bcd_headland_t;

// Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
// Returns a cJSON object owned by the caller.
// Mutates env in-place (preprocessing resolves sweep-axis vertex collisions).
cJSON *coverage_path_planning_process(input_environment_t *env);

// API calls

char *bcd_build_metadata_json(void);
char *bcd_run_compute(const char *input_environment_json);

// POINT_T Helpers

bool are_equal_points(const point_t a, const point_t b);

// 'Destructors'

void free_polygon(polygon_t *polygon);
void free_input_environment(input_environment_t *env);

#endif // INTERNAL_H
