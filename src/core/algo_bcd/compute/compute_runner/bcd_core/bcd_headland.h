#ifndef BCD_HEADLAND_H
#define BCD_HEADLAND_H

#include "../../../internal.h"

/**
 * Computes perimeter-following headland paths for a pre-validated environment.
 *
 * For the zone boundary, the headland path follows the inner offset at
 * path_width/2. For each obstacle, the path follows its outer offset at
 * path_width/2.
 *
 * When the zone offset path comes within path_width/2 of an obstacle's
 * original boundary, generation transfers to that obstacle's offset path.
 * If while tracing the obstacle offset path another obstacle is encountered,
 * generation cascades to that obstacle. Whenever a transferred obstacle's
 * path comes within path_width/2 of the caller polygon's original boundary,
 * control returns to the caller. All reachable obstacles are thus covered.
 * Any remaining unreachable obstacles (isolated from zone contact) receive
 * independent headland sections.
 *
 * On success, headland->sections contains the generated path sections and
 * headland->shrunken_zone / headland->expanded_obstacles contain the reduced
 * geometry for subsequent BCD coverage planning.
 *
 * Returns 0 on success, negative on error.
 */
int compute_bcd_headland(const input_environment_t *env,
                         bcd_headland_t *headland);

/**
 * Frees all memory owned by a bcd_headland_t. Safe to call on a zero-
 * initialised struct.
 */
void free_bcd_headland(bcd_headland_t *headland);

#endif // BCD_HEADLAND_H
