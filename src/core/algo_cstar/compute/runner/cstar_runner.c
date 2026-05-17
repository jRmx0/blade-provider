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
#include "debug/cstar_debug.h"

#include "core/rcg/cstar_rcg.c"
#include "core/preprocess/cstar_lap.c"
#include "core/sampling/cstar_sampling.c"
#include "core/rcg/cstar_rcg_growth.c"
#include "core/waypoint/cstar_waypoint.c"
#include "core/dead_end/cstar_dead_end.c"
#include "core/coverage_hole/cstar_coverage_hole.c"
#include "debug/cstar_debug.c"

#include "utils/cstar_runner_math.c"
#include "utils/cstar_runner_result.c"
#include "utils/cstar_runner_segments.c"

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

    if (!cstar_debug_init(&debug_state))
    {
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        return NULL;
    }

    float w = env->path_width;
    float rd = env->sensor_range;
    int delta = (int)env->frontier_spacing_multiplier;

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
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cstar_result_cleanup_partial(result);
                return NULL;
            }
        }

        if (goal_node_id != CSTAR_NO_NEIGHBOR)
        {
            point_t goal = rcg.nodes[goal_node_id].pos;
            if (!cstar_add_initial_coverage(result, &segment_id, current, goal))
            {
                cstar_debug_dispose(&debug_state);
                cstar_sampling_front_free(&sampling_front);
                cstar_rcg_free(&rcg);
                cstar_result_cleanup_partial(result);
                return NULL;
            }
        }
    }

    bool debug_ok = cstar_debug_export_rcg_nodes(&debug_state, &rcg) &&
                    cstar_debug_export_rcg_edges(&debug_state, &rcg) &&
                    cstar_debug_export_laps(&debug_state, &sampling_front, env) &&
                    cstar_debug_export_sampling_front_polygon(&debug_state, env) &&
                    cstar_debug_finalize_layers(&debug_state, &result->debug_layers);
    if (!debug_ok)
    {
        cstar_debug_dispose(&debug_state);
        cstar_sampling_front_free(&sampling_front);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        return NULL;
    }

    cstar_debug_dispose(&debug_state);
    cstar_sampling_front_free(&sampling_front);
    cstar_rcg_free(&rcg);
    return result;
}
