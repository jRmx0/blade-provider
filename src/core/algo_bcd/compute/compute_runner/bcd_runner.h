// Orchestrates the coverage path planning process

#ifndef COVERAGE_PATH_PLANNING_H
#define COVERAGE_PATH_PLANNING_H

#include "../../internal.h"

// Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
// Returns a newly allocated JSON string; caller must free().
char *coverage_path_planning_process(const input_environment_t *env);

#endif // COVERAGE_PATH_PLANNING_H
