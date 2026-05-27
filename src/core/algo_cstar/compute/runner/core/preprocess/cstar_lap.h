#ifndef CSTAR_LAP_H
#define CSTAR_LAP_H

#include "../sampling/cstar_sampling.h"
#include "../../../../cstar.h"
#include "../geometry/cstar_geometry.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Environment Preprocessing (One-Time Initialization)
// -------------------------------------------------------------------------

/**
 * Generates and stores vertical lap lattice in the environment structure.
 *
 * This is a one-time preprocessing step that must be called before
 * coverage path planning. Laps are anchored at env->start_point and
 * span the full horizontal extent of the environment.
 *
 * Parameters:
 *   env        - Environment to preprocess (must be initialized with valid boundary)
 *   path_width - Lap spacing distance (same as algorithm's path_width parameter)
 *
 * Returns:
 *   true  - Laps successfully generated and stored in env->laps
 *   false - Laps already exist in env->laps (idempotency check failed) or allocation error
 *
 * After successful call, env->laps is populated with a cvector of cstar_lap_t.
 * These laps are owned by the environment and must be freed with cstar_environment_laps_cleanup().
 */
bool cstar_preprocess_environment_laps(cstar_environment_t *env, float path_width);

/**
 * Frees all lap-related memory stored in the environment.
 *
 * Deallocates:
 *   - Each lap's node_ids vector
 *   - The laps vector itself (env->laps)
 *
 * After this call, env->laps will be NULL. Safe to call multiple times.
 */
void cstar_environment_laps_cleanup(cstar_environment_t *env);

// -------------------------------------------------------------------------
// Sampling Helpers
// -------------------------------------------------------------------------

/**
 * Computes the Y-axis sample index range and step for frontier sampling.
 *
 * Wraps the cstar_lap_boundary_bbox call and the ceilf/floorf index
 * calculations that are shared between cstar_generate_frontier_samples and
 * cstar_generate_obstacle_adjacent_samples.
 *
 * Parameters:
 *   env        - Environment (must have valid operationalBoundary and start_point)
 *   w          - Path width used as the base step size
 *   delta      - Frontier spacing multiplier (> 0); 1 if zero or negative
 *   step_out   - Receives the computed step value
 *   anchor_y_out - Receives env->start_point.y (anchor for index-to-y mapping)
 *   first_out  - Receives the lowest sample index (inclusive)
 *   last_out   - Receives the highest sample index (inclusive)
 */
void cstar_lap_compute_sample_range(const cstar_environment_t *env,
                                    float w, int delta,
                                    float *step_out, float *anchor_y_out,
                                    int *first_out, int *last_out);

#endif // CSTAR_LAP_H
