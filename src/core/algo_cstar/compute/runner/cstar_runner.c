#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "cstar_runner.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"
#include "../../../common/debug_serialize.h"
#include "core/cstar_rcg.h"
#include "core/preprocess/cstar_lap.h"
#include "core/cstar_sampling.h"
#include "core/cstar_rcg_growth.h"
#include "core/cstar_waypoint.h"
#include "core/cstar_dead_end.h"
#include "core/cstar_coverage_hole.h"

#include "core/cstar_rcg.c"
#include "core/preprocess/cstar_lap.c"
#include "core/cstar_sampling.c"
#include "core/cstar_rcg_growth.c"
#include "core/cstar_waypoint.c"
#include "core/cstar_dead_end.c"
#include "core/cstar_coverage_hole.c"

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

static void cstar_runner_add_point_entry(cJSON *arr, int id, point_t p, const char *label)
{
    cJSON *entry = cJSON_CreateObject();
    cJSON *point = cJSON_CreateObject();
    if (entry == NULL || point == NULL)
    {
        cJSON_Delete(entry);
        cJSON_Delete(point);
        return;
    }

    cJSON_AddNumberToObject(entry, "id", id);
    cJSON_AddNumberToObject(point, "x", p.x);
    cJSON_AddNumberToObject(point, "y", p.y);
    cJSON_AddItemToObject(entry, "point", point);
    if (label != NULL)
    {
        cJSON_AddStringToObject(entry, "pointLabel", label);
    }
    cJSON_AddItemToArray(arr, entry);
}

static bool cstar_add_debug_layer(cJSON *layers_arr, int id, const char *source, const cJSON *list)
{
    if (layers_arr == NULL || source == NULL || list == NULL)
    {
        return false;
    }

    cJSON *layer = cJSON_CreateObject();
    cJSON *list_copy = cJSON_Duplicate((cJSON *)list, 1);
    if (layer == NULL || list_copy == NULL)
    {
        cJSON_Delete(layer);
        cJSON_Delete(list_copy);
        return false;
    }

    cJSON_AddNumberToObject(layer, "id", id);
    cJSON_AddStringToObject(layer, "source", source);
    cJSON_AddItemToObject(layer, "list", list_copy);
    cJSON_AddItemToArray(layers_arr, layer);
    return true;
}

static bool cstar_append_segment(cJSON *segments_arr,
                                 cJSON *typed_arr,
                                 int *segment_id,
                                 const char *type,
                                 const float *xs,
                                 const float *ys,
                                 int count)
{
    cJSON *segment = debug_build_segment(*segment_id, type, xs, ys, count);
    if (segment == NULL)
    {
        return false;
    }

    cJSON *typed_copy = cJSON_Duplicate(segment, 1);
    if (typed_copy == NULL)
    {
        cJSON_Delete(segment);
        return false;
    }

    cJSON_AddItemToArray(segments_arr, segment);
    cJSON_AddItemToArray(typed_arr, typed_copy);
    (*segment_id)++;
    return true;
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
            float xs[2] = {env->start_point.x, current.x};
            float ys[2] = {env->start_point.y, current.y};
            if (!cstar_append_segment(segments, coverage_transit, &segment_id, "coverageTransit", xs, ys, 2))
            {
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cJSON_Delete(root);
                return cstar_create_runner_error("allocation_failed", "C* failed while building start transit segment.");
            }
        }

        if (goal_node_id != CSTAR_NO_NEIGHBOR)
        {
            point_t goal = rcg.nodes[goal_node_id].pos;
            float xs[2] = {current.x, goal.x};
            float ys[2] = {current.y, goal.y};
            if (!cstar_append_segment(segments, coverage, &segment_id, "coverage", xs, ys, 2))
            {
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cJSON_Delete(root);
                return cstar_create_runner_error("allocation_failed", "C* failed while building first coverage segment.");
            }
        }
    }

    cJSON *rcg_link_nodes = cJSON_CreateArray();
    cJSON *rcg_end_nodes = cJSON_CreateArray();
    cJSON *rcg_edges = cJSON_CreateArray();
    cJSON *lap_list = cJSON_CreateArray();
    cJSON *sampling_front_list = cJSON_CreateArray();
    cJSON *frontier_sample_list = cJSON_CreateArray();
    cJSON *retreat_node_list = cJSON_CreateArray();
    cJSON *coverage_hole_list = cJSON_CreateArray();

    cJSON_AddItemToObject(root, "rcgLinkNodeList", rcg_link_nodes);
    cJSON_AddItemToObject(root, "rcgEndNodeList", rcg_end_nodes);
    cJSON_AddItemToObject(root, "rcgEdgeList", rcg_edges);
    cJSON_AddItemToObject(root, "lapList", lap_list);
    cJSON_AddItemToObject(root, "samplingFrontList", sampling_front_list);
    cJSON_AddItemToObject(root, "frontierSampleList", frontier_sample_list);
    cJSON_AddItemToObject(root, "retreatNodeList", retreat_node_list);
    cJSON_AddItemToObject(root, "coverageHoleList", coverage_hole_list);

    for (int i = 0; i < rcg.node_count; ++i)
    {
        cstar_runner_add_point_entry(frontier_sample_list, i + 1, rcg.nodes[i].pos, NULL);
        if (rcg.nodes[i].is_end_node)
        {
            cstar_runner_add_point_entry(rcg_end_nodes, i + 1, rcg.nodes[i].pos, NULL);
        }
        if (rcg.nodes[i].is_link_node)
        {
            cstar_runner_add_point_entry(rcg_link_nodes, i + 1, rcg.nodes[i].pos, NULL);
        }
    }

    for (int i = 0; i < rcg.edge_count; ++i)
    {
        const cstar_edge_t *edge = &rcg.edges[i];
        point_t a = rcg.nodes[edge->node_a].pos;
        point_t b = rcg.nodes[edge->node_b].pos;
        float xs[2] = {a.x, b.x};
        float ys[2] = {a.y, b.y};
        cJSON *segment = debug_build_segment(i + 1, "rcgEdge", xs, ys, 2);
        if (segment != NULL)
        {
            cJSON_AddItemToArray(rcg_edges, segment);
        }
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);
    if (sampling_front.laps != NULL)
    {
        int lap_count = (int)cvector_size(sampling_front.laps);
        for (int i = 0; i < lap_count; ++i)
        {
            float x = sampling_front.laps[i].x;
            float xs[2] = {x, x};
            float ys[2] = {min_y, max_y};
            cJSON *segment = debug_build_segment(i + 1, "lap", xs, ys, 2);
            if (segment != NULL)
            {
                cJSON_AddItemToArray(lap_list, segment);
            }
        }
    }

    cJSON *front_polygon = cJSON_CreateObject();
    cJSON *front_vertices = cJSON_CreateArray();
    if (front_polygon != NULL && front_vertices != NULL)
    {
        cJSON_AddNumberToObject(front_polygon, "id", 1);
        cJSON_AddItemToObject(front_polygon, "vertices", front_vertices);
        for (uint32_t i = 0; i < env->boundary.vertex_count; ++i)
        {
            cJSON *jv = cJSON_CreateObject();
            cJSON_AddNumberToObject(jv, "x", env->boundary.vertices[i].x);
            cJSON_AddNumberToObject(jv, "y", env->boundary.vertices[i].y);
            cJSON_AddItemToArray(front_vertices, jv);
        }
        cJSON_AddItemToArray(sampling_front_list, front_polygon);
    }
    else
    {
        cJSON_Delete(front_polygon);
        cJSON_Delete(front_vertices);
    }

    cJSON *debug = cJSON_CreateObject();
    cJSON *layers = cJSON_CreateArray();
    if (debug != NULL && layers != NULL)
    {
        bool debug_ok = true;
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 10, "rcgLinkNodeList", rcg_link_nodes);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 11, "rcgEndNodeList", rcg_end_nodes);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 12, "rcgEdgeList", rcg_edges);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 13, "lapList", lap_list);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 14, "samplingFrontList", sampling_front_list);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 15, "frontierSampleList", frontier_sample_list);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 16, "retreatNodeList", retreat_node_list);
        debug_ok = debug_ok && cstar_add_debug_layer(layers, 17, "coverageHoleList", coverage_hole_list);

        if (debug_ok)
        {
            cJSON_AddItemToObject(debug, "layers", layers);
            cJSON_AddItemToObject(root, "debug", debug);
            debug = NULL;
            layers = NULL;
        }
    }

    cJSON_Delete(debug);
    cJSON_Delete(layers);

    cstar_sampling_front_free(&sampling_front);
    cstar_rcg_free(&rcg);
    return root;
}
