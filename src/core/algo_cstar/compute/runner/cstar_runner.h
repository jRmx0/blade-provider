// Orchestrates the C* coverage path planning process

#ifndef CSTAR_RUNNER_H
#define CSTAR_RUNNER_H

#include "../../internal.h"
#include "../../../../../dependencies/cJSON/cJSON.h"

/**
 * Runs the C* computation pipeline on a pre-validated, pre-parsed environment.
 * Returns a cJSON object owned by the caller; hand off to cstar_run_compute
 * for serialisation and cleanup.
 */
cJSON *cstar_coverage_path_planning_process(input_environment_t *env);

#endif // CSTAR_RUNNER_H
