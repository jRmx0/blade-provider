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
#include "core/geometry/cstar_geometry.c"
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

    // One-time preprocessing: generate and store laps in environment
    if (!cstar_preprocess_environment_laps(env, w))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    int delta = (env->frontier_spacing_multiplier > 0u)
                    ? (int)env->frontier_spacing_multiplier
                    : 1;

    int generated_samples = cstar_generate_frontier_samples(&rcg,
                                                            w,
                                                            delta,
                                                            env);
    if (generated_samples <= 0)
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    if (!cstar_debug_export_laps(&debug_state, env) ||
        !cstar_debug_export_rcg_nodes(&debug_state, &rcg) ||
        !cstar_debug_finalize_layers(&debug_state, &result->debug_layers))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    cstar_debug_dispose(&debug_state);
    cstar_rcg_free(&rcg);
    cstar_environment_laps_cleanup(env);
    return result;
}
