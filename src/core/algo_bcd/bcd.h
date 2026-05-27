/**
 * bcd.h
 *
 * Public interface for the Boustrophedon Cellular Decomposition algorithm.
 * The only header exposed outside of this module.
 * Declares the two entry points: metadata and compute, plus shared internal
 * types used across all bcd sub-modules.
 *
 * Included by: dispatcher.c, and all bcd sub-modules.
 */

#ifndef BCD_H
#define BCD_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../core_types.h"

/* Forward declaration — full struct definition is in compute/runner/utils/bcd_runner_result.c */
typedef struct bcd_result_t bcd_result_t;

// API calls

char *bcd_build_metadata_json(void);
bcd_result_t *bcd_run_compute(input_environment_t *environment);

// POINT_T Helpers

bool are_equal_points(const point_t a, const point_t b);

// 'Destructors'

void free_polygon(polygon_t *polygon);
void free_input_environment(input_environment_t *env);

// Public entry points

char *bcd_get_metadata_json(void);
char *bcd_compute(const char *input_environment_json);

#endif // BCD_H