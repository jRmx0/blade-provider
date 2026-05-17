#include "cstar_runner_segments.h"
#include "cstar_runner_result.h"

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
