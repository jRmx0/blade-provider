// Orchestrates the C* coverage path planning process

#ifndef CSTAR_RUNNER_H
#define CSTAR_RUNNER_H

#include "../../internal.h"

/**
 * Runs the C* computation pipeline on a pre-validated, pre-parsed environment.
 * Returns a coverage path result struct owned by the caller; caller must free
 * with cstar_result_free() (defined in compute/serializer/cstar_serializer.h).
 */
cstar_coverage_path_result_t *cstar_coverage_path_planning_process(cstar_environment_t *env);

#endif // CSTAR_RUNNER_H
