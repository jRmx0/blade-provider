#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "cstar_runner.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "core/cstar_rcg.h"
#include "core/preprocess/cstar_lap.h"
#include "core/cstar_sampling.h"
#include "core/cstar_rcg_growth.h"
#include "core/cstar_waypoint.h"
#include "core/cstar_dead_end.h"
#include "core/cstar_coverage_hole.h"
#include "output/debug/cstar_debug.h"
#include "output/segments/cstar_segments.h"

#include "core/cstar_rcg.c"
#include "core/preprocess/cstar_lap.c"
#include "core/cstar_sampling.c"
#include "core/cstar_rcg_growth.c"
#include "core/cstar_waypoint.c"
#include "core/cstar_dead_end.c"
#include "core/cstar_coverage_hole.c"
#include "output/debug/cstar_debug.c"
#include "output/segments/cstar_segments.c"

static cJSON *cstar_create_runner_error(const char *code, const char *message)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "code", code != NULL ? code : "cstar_error");
    cJSON_AddStringToObject(root, "message", message != NULL ? message : "C* runner failed.");
    return root;
}

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

cJSON *cstar_coverage_path_planning_process(input_environment_t *env)
{
    if (env == NULL)
    {
        return cstar_create_runner_error("invalid_environment", "C* requires a valid environment.");
    }

    if (env->boundary.vertex_count < 3u || env->boundary.vertices == NULL)
    {
        return cstar_create_runner_error("invalid_boundary", "C* boundary must contain at least 3 vertices.");
    }

    cstar_rcg_t rcg;
    cstar_rcg_init(&rcg);
    cstar_sampling_front_t sampling_front = {0};
    cstar_debug_t debug_state = {0};

    cJSON *root = cJSON_CreateObject();
    cJSON *coverage_path_plan = cJSON_CreateObject();
    cJSON *segments = cJSON_CreateArray();
    cJSON *coverage = cJSON_CreateArray();
    cJSON *coverage_transit = cJSON_CreateArray();
    cJSON *retreat_transit = cJSON_CreateArray();
    cJSON *hole_coverage = cJSON_CreateArray();
    cJSON *hole_transit = cJSON_CreateArray();

    if (root == NULL || coverage_path_plan == NULL || segments == NULL ||
        coverage == NULL || coverage_transit == NULL || retreat_transit == NULL ||
        hole_coverage == NULL || hole_transit == NULL)
    {
        cJSON_Delete(root);
        cJSON_Delete(coverage_path_plan);
        cJSON_Delete(segments);
        cJSON_Delete(coverage);
        cJSON_Delete(coverage_transit);
        cJSON_Delete(retreat_transit);
        cJSON_Delete(hole_coverage);
        cJSON_Delete(hole_transit);
        cstar_rcg_free(&rcg);
        return cstar_create_runner_error("allocation_failed", "C* failed to allocate result JSON.");
    }

    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddItemToObject(root, "coveragePathPlan", coverage_path_plan);
    cJSON_AddItemToObject(coverage_path_plan, "segments", segments);
    cJSON_AddItemToObject(coverage_path_plan, "coverage", coverage);
    cJSON_AddItemToObject(coverage_path_plan, "coverageTransit", coverage_transit);
    cJSON_AddItemToObject(coverage_path_plan, "retreatTransit", retreat_transit);
    cJSON_AddItemToObject(coverage_path_plan, "holeCoverage", hole_coverage);
    cJSON_AddItemToObject(coverage_path_plan, "holeTransit", hole_transit);

    if (!cstar_debug_init(&debug_state, root))
    {
        cJSON_Delete(root);
        cstar_rcg_free(&rcg);
        return cstar_create_runner_error("allocation_failed", "C* failed to allocate debug JSON.");
    }

    float w = (env->path_width > 0.0f) ? env->path_width : 1.0f;
    float rd = (env->target_distance > 0.0f) ? env->target_distance : w;
    int delta = (env->max_iterations > 0u) ? (int)env->max_iterations : 1;

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
            if (!cstar_segments_add_start_transit(segments, coverage_transit, &segment_id, env->start_point, current))
            {
                cstar_debug_dispose(&debug_state);
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cJSON_Delete(root);
                return cstar_create_runner_error("allocation_failed", "C* failed while building start transit segment.");
            }
        }

        if (goal_node_id != CSTAR_NO_NEIGHBOR)
        {
            point_t goal = rcg.nodes[goal_node_id].pos;
            if (!cstar_segments_add_initial_coverage(segments, coverage, &segment_id, current, goal))
            {
                cstar_debug_dispose(&debug_state);
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cJSON_Delete(root);
                return cstar_create_runner_error("allocation_failed", "C* failed while building first coverage segment.");
            }
        }
    }

    bool debug_ok = cstar_debug_export_rcg_nodes(&debug_state, &rcg) &&
                    cstar_debug_export_rcg_edges(&debug_state, &rcg) &&
                    cstar_debug_export_laps(&debug_state, &sampling_front, env) &&
                    cstar_debug_export_sampling_front_polygon(&debug_state, env) &&
                    cstar_debug_attach_layers(&debug_state, root);
    if (!debug_ok)
    {
        cstar_debug_dispose(&debug_state);
        cstar_sampling_front_free(&sampling_front);
        cstar_rcg_free(&rcg);
        cJSON_Delete(root);
        return cstar_create_runner_error("allocation_failed", "C* failed while building debug JSON.");
    }

    cstar_debug_dispose(&debug_state);
    cstar_sampling_front_free(&sampling_front);
    cstar_rcg_free(&rcg);
    return root;
}
