// Orchestrates the coverage path planning process

#ifndef COVERAGE_PATH_PLANNING_H
#define COVERAGE_PATH_PLANNING_H

#include <stdint.h>
#include <stdbool.h>

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

typedef enum {
    POLYGON_WINDING_UNKNOWN = 0,
    POLYGON_WINDING_CW = 1,         // Boundary winding type
    POLYGON_WINDING_CCW = 2         // Obstacle winding type
} polygon_winding_t;

typedef enum {
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

    polygon_t boundary;

    polygon_t *obstacles;
    uint32_t obstacle_count;
} input_environment_t;

// Processes the input environment JSON and returns a newly allocated JSON string
// with shape: { "status": "ok", "event_list": [ ... ], "cell_list": [ ... ], "path_list": [ ... ], "motion_plan": { ... } } on success, or
// { "status": "error", "message": "..." } on failure. Caller must free().
char *coverage_path_planning_process(const char *input_environment_json);

// POINT_T Helpers

bool are_equal_points(const point_t a, const point_t b);

// 'Destructors'

void free_polygon(polygon_t *polygon);
void free_input_environment(input_environment_t *env);

#endif // COVERAGE_PATH_PLANNING_H
