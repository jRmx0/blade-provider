#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cstar_runner.h"
#include "core/rcg/cstar_rcg.h"
#include "core/preprocess/cstar_lap.h"
#include "core/sampling/cstar_sampling.h"
#include "core/rcg/cstar_rcg_growth.h"
#include "core/waypoint/cstar_waypoint.h"
#include "core/dead_end/cstar_dead_end.h"
#include "core/coverage_hole/cstar_coverage_hole.h"
#include "output/debug/cstar_debug.h"

#include "core/rcg/cstar_rcg.c"
#include "core/preprocess/cstar_lap.c"
#include "core/sampling/cstar_sampling.c"
#include "core/rcg/cstar_rcg_growth.c"
#include "core/waypoint/cstar_waypoint.c"
#include "core/dead_end/cstar_dead_end.c"
#include "core/coverage_hole/cstar_coverage_hole.c"
#include "output/debug/cstar_debug.c"

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

static bool cstar_points_equal(point_t a, point_t b)
{
    return fabsf(a.x - b.x) < 1e-5f && fabsf(a.y - b.y) < 1e-5f;
}

static float cstar_runner_dist(point_t a, point_t b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

// -------------------------------------------------------------------------
// Result Struct Allocation & Management
// -------------------------------------------------------------------------

static cstar_coverage_path_result_t *cstar_result_create(void)
{
    cstar_coverage_path_result_t *result =
        (cstar_coverage_path_result_t *)malloc(sizeof(cstar_coverage_path_result_t));
    if (result == NULL)
    {
        return NULL;
    }

    result->all_segments = NULL;
    result->segment_count = 0;
    result->segment_capacity = 0;

    result->coverage.segments = NULL;
    result->coverage.segment_count = 0;
    result->coverage.segment_capacity = 0;

    result->coverage_transit.segments = NULL;
    result->coverage_transit.segment_count = 0;
    result->coverage_transit.segment_capacity = 0;

    result->retreat_transit.segments = NULL;
    result->retreat_transit.segment_count = 0;
    result->retreat_transit.segment_capacity = 0;

    result->hole_coverage.segments = NULL;
    result->hole_coverage.segment_count = 0;
    result->hole_coverage.segment_capacity = 0;

    result->hole_transit.segments = NULL;
    result->hole_transit.segment_count = 0;
    result->hole_transit.segment_capacity = 0;

    result->debug_layers = NULL;

    return result;
}

/**
 * Add a segment to the result struct and to its appropriate collection.
 * Makes a copy of the segment data.
 */
static bool cstar_result_add_segment(cstar_coverage_path_result_t *result,
                                     int segment_id,
                                     const char *type,
                                     const point_t *path,
                                     int path_count)
{
    if (result == NULL || type == NULL || path == NULL || path_count <= 0)
    {
        return false;
    }

    // Grow segments array if needed
    if (result->segment_count >= result->segment_capacity)
    {
        int new_capacity = result->segment_capacity == 0 ? 16 : result->segment_capacity * 2;
        cstar_segment_t *new_segments =
            (cstar_segment_t *)realloc(result->all_segments, new_capacity * sizeof(cstar_segment_t));
        if (new_segments == NULL)
        {
            return false;
        }
        result->all_segments = new_segments;
        result->segment_capacity = new_capacity;
    }

    // Allocate and populate segment
    cstar_segment_t *seg = &result->all_segments[result->segment_count];
    seg->id = segment_id;

    seg->type = (char *)malloc(strlen(type) + 1);
    if (seg->type == NULL)
    {
        return false;
    }
    strcpy(seg->type, type);

    seg->path = (cstar_path_point_t *)malloc(path_count * sizeof(cstar_path_point_t));
    if (seg->path == NULL)
    {
        free(seg->type);
        return false;
    }

    for (int i = 0; i < path_count; ++i)
    {
        seg->path[i].id = i;
        seg->path[i].point = path[i];
    }
    seg->path_count = path_count;

    result->segment_count++;

    return true;
}

// -------------------------------------------------------------------------
// Segment Builders
// -------------------------------------------------------------------------

/**
 * Add a transit segment from start point to current point.
 */
static bool cstar_add_start_transit(cstar_coverage_path_result_t *result,
                                    int *segment_id,
                                    point_t start_point,
                                    point_t current_point)
{
    point_t path[2] = {start_point, current_point};
    if (!cstar_result_add_segment(result, *segment_id, "coverageTransit", path, 2))
    {
        return false;
    }
    (*segment_id)++;
    return true;
}

/**
 * Add an initial coverage segment from current point to goal point.
 */
static bool cstar_add_initial_coverage(cstar_coverage_path_result_t *result,
                                       int *segment_id,
                                       point_t current_point,
                                       point_t goal_point)
{
    point_t path[2] = {current_point, goal_point};
    if (!cstar_result_add_segment(result, *segment_id, "coverage", path, 2))
    {
        return false;
    }
    (*segment_id)++;
    return true;
}

cstar_coverage_path_result_t *cstar_coverage_path_planning_process(cstar_environment_t *env)
{
    if (env == NULL)
    {
        return NULL;
    }

    if (env->operationalBoundary.vertex_count < 3u || env->operationalBoundary.vertices == NULL)
    {
        return NULL;
    }

    cstar_rcg_t rcg;
    cstar_rcg_init(&rcg);
    cstar_sampling_front_t sampling_front = {0};
    cstar_debug_t debug_state = {0};

    cstar_coverage_path_result_t *result = cstar_result_create();
    if (result == NULL)
    {
        cstar_rcg_free(&rcg);
        return NULL;
    }

    // Initialize debug state with a temporary JSON object (will be extracted after computation)
    cJSON *debug_root = cJSON_CreateObject();
    if (debug_root == NULL || !cstar_debug_init(&debug_state, debug_root))
    {
        cJSON_Delete(debug_root);
        cstar_rcg_free(&rcg);
        // TODO: implement cstar_result_free in cstar_serializer.h
        return NULL;
    }

    float w = (env->path_width > 0.0f) ? env->path_width : 1.0f;
    float rd = (env->sensor_range > 0.0f) ? env->sensor_range : w;
    int delta = 1;

    point_t lap_dir = {0.0f, 1.0f};
    sampling_front = cstar_create_sampling_front(env->start_point,
                                                 env->start_point,
                                                 rd,
                                                 w,
                                                 lap_dir,
                                                 env);
    cstar_generate_frontier_samples(&sampling_front, &rcg, w, delta, env);
    cstar_rcg_expand(&rcg, &sampling_front, w, env);

    int current_node_id = CSTAR_NO_NEIGHBOR;
    float best_dist = INFINITY;
    for (int i = 0; i < rcg.node_count; ++i)
    {
        if (rcg.nodes[i].state != CSTAR_NODE_OP)
            continue;

        float d = cstar_runner_dist(env->start_point, rcg.nodes[i].pos);
        if (d < best_dist)
        {
            best_dist = d;
            current_node_id = i;
        }
    }

    int goal_node_id = CSTAR_NO_NEIGHBOR;
    if (current_node_id != CSTAR_NO_NEIGHBOR)
    {
        goal_node_id = cstar_select_goal_node(&rcg, current_node_id);
    }

    int segment_id = 0;
    if (current_node_id != CSTAR_NO_NEIGHBOR)
    {
        point_t current = rcg.nodes[current_node_id].pos;
        if (!cstar_points_equal(env->start_point, current))
        {
            if (!cstar_add_start_transit(result, &segment_id, env->start_point, current))
            {
                cstar_debug_dispose(&debug_state);
                cJSON_Delete(debug_root);
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                // TODO: implement cstar_result_free
                return NULL;
            }
        }

        if (goal_node_id != CSTAR_NO_NEIGHBOR)
        {
            point_t goal = rcg.nodes[goal_node_id].pos;
            if (!cstar_add_initial_coverage(result, &segment_id, current, goal))
            {
                cstar_debug_dispose(&debug_state);
                cJSON_Delete(debug_root);
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                // TODO: implement cstar_result_free
                return NULL;
            }
        }
    }

    bool debug_ok = cstar_debug_export_rcg_nodes(&debug_state, &rcg) &&
                    cstar_debug_export_rcg_edges(&debug_state, &rcg) &&
                    cstar_debug_export_laps(&debug_state, &sampling_front, env) &&
                    cstar_debug_export_sampling_front_polygon(&debug_state, env) &&
                    cstar_debug_attach_layers(&debug_state, debug_root);
    if (!debug_ok)
    {
        cstar_debug_dispose(&debug_state);
        cJSON_Delete(debug_root);
        cstar_sampling_front_free(&sampling_front);
        cstar_rcg_free(&rcg);
        // TODO: implement cstar_result_free
        return NULL;
    }

    // Extract debug layers from root before cleanup
    cJSON *debug_layers = cJSON_GetObjectItemCaseSensitive(debug_root, "debug");
    if (debug_layers != NULL)
    {
        result->debug_layers = cJSON_Duplicate(debug_layers, 1);
    }

    cstar_debug_dispose(&debug_state);
    cJSON_Delete(debug_root);
    cstar_sampling_front_free(&sampling_front);
    cstar_rcg_free(&rcg);
    return result;
}
