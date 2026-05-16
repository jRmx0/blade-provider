#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "cstar_runner.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"
#include "../../../common/debug_serialize.h"
#include "core/cstar_rcg.h"
#include "core/cstar_sampling.h"
#include "core/cstar_rcg_growth.h"
#include "core/cstar_waypoint.h"
#include "core/cstar_dead_end.h"
#include "core/cstar_coverage_hole.h"

#include "core/cstar_rcg.c"
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

    int segment_id = 0;
    point_t start = env->start_point;
    point_t first_boundary = env->boundary.vertices[0];

    if (!cstar_points_equal(start, first_boundary))
    {
        float xs[2] = {start.x, first_boundary.x};
        float ys[2] = {start.y, first_boundary.y};
        if (!cstar_append_segment(segments, coverage_transit, &segment_id, "coverageTransit", xs, ys, 2))
        {
            cJSON_Delete(root);
            return cstar_create_runner_error("allocation_failed", "C* failed while building transit segment.");
        }
    }

    int boundary_count = (int)env->boundary.vertex_count;
    int coverage_point_count = boundary_count + 1;

    float *coverage_xs = (float *)malloc((size_t)coverage_point_count * sizeof(float));
    float *coverage_ys = (float *)malloc((size_t)coverage_point_count * sizeof(float));
    if (coverage_xs == NULL || coverage_ys == NULL)
    {
        free(coverage_xs);
        free(coverage_ys);
        cJSON_Delete(root);
        return cstar_create_runner_error("allocation_failed", "C* failed to allocate coverage polyline.");
    }

    for (int i = 0; i < boundary_count; ++i)
    {
        coverage_xs[i] = env->boundary.vertices[i].x;
        coverage_ys[i] = env->boundary.vertices[i].y;
    }
    coverage_xs[boundary_count] = env->boundary.vertices[0].x;
    coverage_ys[boundary_count] = env->boundary.vertices[0].y;

    if (!cstar_append_segment(segments, coverage, &segment_id, "coverage", coverage_xs, coverage_ys, coverage_point_count))
    {
        free(coverage_xs);
        free(coverage_ys);
        cJSON_Delete(root);
        return cstar_create_runner_error("allocation_failed", "C* failed while building coverage segment.");
    }

    point_t end = env->end_point;
    point_t last_coverage = env->boundary.vertices[0];
    if (!cstar_points_equal(last_coverage, end))
    {
        float xs[2] = {last_coverage.x, end.x};
        float ys[2] = {last_coverage.y, end.y};
        if (!cstar_append_segment(segments, coverage_transit, &segment_id, "coverageTransit", xs, ys, 2))
        {
            free(coverage_xs);
            free(coverage_ys);
            cJSON_Delete(root);
            return cstar_create_runner_error("allocation_failed", "C* failed while building end transit segment.");
        }
    }

    free(coverage_xs);
    free(coverage_ys);

    cJSON_AddItemToObject(root, "rcgLinkNodeList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "rcgEndNodeList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "rcgEdgeList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "lapList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "samplingFrontList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "frontierSampleList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "retreatNodeList", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "coverageHoleList", cJSON_CreateArray());

    cJSON *debug = cJSON_CreateObject();
    cJSON *layers = cJSON_CreateArray();
    if (debug != NULL && layers != NULL)
    {
        cJSON_AddItemToObject(debug, "layers", layers);
        cJSON_AddItemToObject(root, "debug", debug);
    }
    else
    {
        cJSON_Delete(debug);
        cJSON_Delete(layers);
    }

    return root;
}
