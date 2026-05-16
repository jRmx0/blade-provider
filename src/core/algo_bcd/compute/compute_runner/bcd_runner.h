// Orchestrates the coverage path planning process

#ifndef COVERAGE_PATH_PLANNING_H
#define COVERAGE_PATH_PLANNING_H

#include "../../internal.h"
#include "../../../../../dependencies/cJSON/cJSON.h"

// Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
// Returns a cJSON object owned by the caller; hand off to bcd_run_compute for
// performance injection, serialization, and cleanup.
// Mutates env in-place (preprocessing resolves sweep-axis vertex collisions).
cJSON *coverage_path_planning_process(input_environment_t *env);

#endif // COVERAGE_PATH_PLANNING_H
