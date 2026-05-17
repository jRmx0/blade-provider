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

#endif // CSTAR_LAP_H
