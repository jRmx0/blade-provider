/**
 * internal.h
 *
 * Private shared definitions for the BCD algorithm module.
 * Contains BCD-specific internal structs, types, constants, and helper
 * function signatures used across metadata/bcd_metadata.c and compute/ files.
 *
 * Fundamental geometry and environment types (point_t, polygon_t,
 * input_environment_t, etc.) are in src/core/core_types.h, included below.
 *
 * Included by: metadata/bcd_metadata.c, bcd_compute.c, step files
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../core_types.h"

// API calls

char *bcd_build_metadata_json(void);
char *bcd_run_compute(const char *input_environment_json);

// POINT_T Helpers

bool are_equal_points(const point_t a, const point_t b);

// 'Destructors'

void free_polygon(polygon_t *polygon);
void free_input_environment(input_environment_t *env);

#endif // INTERNAL_H
