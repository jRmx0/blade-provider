#ifndef CSTAR_RUNNER_RESULT_H
#define CSTAR_RUNNER_RESULT_H

#include "../cstar_runner.h"
#include "../../../../core_types.h"

/**
 * Create an empty result struct with initialized segment collections.
 */
static cstar_coverage_path_result_t *cstar_result_create(void);

/**
 * Add a segment to the result struct and to its appropriate collection.
 * Makes a copy of the segment data.
 */
static bool cstar_result_add_segment(cstar_coverage_path_result_t *result,
                                     int segment_id,
                                     const char *type,
                                     const point_t *path,
                                     int path_count);

/**
 * Clean up and free all allocated memory in the result struct.
 */
static void cstar_result_cleanup_partial(cstar_coverage_path_result_t *result);

#endif // CSTAR_RUNNER_RESULT_H
