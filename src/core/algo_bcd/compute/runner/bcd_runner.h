// Orchestrates the coverage path planning process

#ifndef COVERAGE_PATH_PLANNING_H
#define COVERAGE_PATH_PLANNING_H

#include "../../bcd.h"

/* Opaque result type — full struct definition lives in bcd_runner.c after all
 * algorithm type headers are included. bcd_compute.c gets the complete type via
 * the C amalgamation include of bcd_runner.c. */
typedef struct bcd_result_t bcd_result_t;

/* Error info populated by coverage_path_planning_process on failure. */
typedef struct
{
    const char *code;
    const char *message;
} bcd_compute_error_t;

/* Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
 * Returns a heap-allocated bcd_result_t on success (caller must call bcd_result_free).
 * Returns NULL on failure; err_out is populated with a static error code and message.
 * Mutates env in-place (preprocessing resolves sweep-axis vertex collisions). */
bcd_result_t *coverage_path_planning_process(input_environment_t *env, bcd_compute_error_t *err_out);

/* Builds the cJSON result tree from a successful bcd_result_t.
 * Returns a cJSON object owned by the caller (must be cJSON_Delete'd). */
#include "../../serialize/bcd_serialize.h"

/* bcd_result_create / bcd_result_free are declared and defined as static in
 * bcd_runner_result.h / bcd_runner_result.c, included by bcd_runner.c. */

#endif // COVERAGE_PATH_PLANNING_H
