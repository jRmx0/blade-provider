/**
 * internal.h
 *
 * Private shared definitions for the Bounce algorithm module.
 * Contains Bounce-specific internal function signatures used across
 * metadata and compute files.
 *
 * Fundamental geometry and environment types (point_t, polygon_t,
 * input_environment_t, etc.) are in src/core/core_types.h, included below.
 *
 * Included by: check/bounce_check.c, bounce_compute.c
 */

#ifndef BOUNCE_INTERNAL_H
#define BOUNCE_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../core_types.h"

// API calls

char *bounce_build_metadata_json(void);
char *bounce_run_compute(const char *input_environment_json);

// Internal pipeline entry point.
// Runs the full Bounce pipeline on a pre-validated environment.
// Returns a cJSON result object owned by the caller (free with cJSON_Delete).
// Returns NULL only on catastrophic allocation failure.
cJSON *bounce_run_pipeline(input_environment_t *environment);

#endif // BOUNCE_INTERNAL_H
