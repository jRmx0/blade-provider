#ifndef CSTAR_RUNNER_SEGMENTS_H
#define CSTAR_RUNNER_SEGMENTS_H

#include "../cstar_runner.h"
#include "../../../../core_types.h"

/**
 * Add a transit segment from start point to current point.
 */
static bool cstar_add_start_transit(cstar_coverage_path_result_t *result,
                                    int *segment_id,
                                    point_t start_point,
                                    point_t current_point);

/**
 * Add an initial coverage segment from current point to goal point.
 */
static bool cstar_add_initial_coverage(cstar_coverage_path_result_t *result,
                                       int *segment_id,
                                       point_t current_point,
                                       point_t goal_point);

#endif // CSTAR_RUNNER_SEGMENTS_H
