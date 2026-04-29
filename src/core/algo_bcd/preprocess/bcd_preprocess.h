/**
 * bcd_preprocess.h
 *
 * Pre-processing step for the BCD algorithm pipeline.
 * Detects and resolves sweep-axis vertex collisions in the input environment
 * before the event list is built.
 *
 * Collisions (two or more vertices with the same projected coordinate along the
 * sweep axis) cause undefined sort order in the event list, which corrupts the
 * cell decomposition.  The preprocessor resolves them by nudging colliding
 * vertices apart along the sweep axis by small incremental offsets.
 *
 * Must be called before build_bcd_event_list().
 * Mutates the environment in-place (vertices and adjacent edges).
 */

#ifndef BCD_PREPROCESS_H
#define BCD_PREPROCESS_H

#include "../internal.h"

/**
 * bcd_preprocess_environment
 *
 * Detects sweep-axis vertex collisions across all polygons in `env` and
 * nudges colliding vertices apart along the sweep direction.
 *
 * @param env                 The input environment to mutate in-place.
 * @param sweep_direction_deg The direction the sweep line travels, in degrees.
 *                            0° = sweep along +x (current BCD default).
 *
 * @return 0 on success, negative on error:
 *   -1  NULL env
 *   -4  allocation failure
 */
int bcd_preprocess_environment(input_environment_t *env, float sweep_direction_deg);

#endif // BCD_PREPROCESS_H
