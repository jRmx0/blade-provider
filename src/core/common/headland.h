#ifndef HEADLAND_H
#define HEADLAND_H

#include "../core_types.h"

/**
 * A single perimeter-following headland path section.
 *
 * source_index: -1 = zone boundary, 0+ = obstacle index.
 * path: the coverage path for this section (closed loop — last point == first point).
 * nav:  transit path to the next section's start point (or to the first
 *       coverage waypoint for the last section). Filled in after all sections
 *       are traced.
 */
typedef struct
{
    cvector_vector_type(point_t) path;
    cvector_vector_type(point_t) nav; // transit to next section (or to first coverage point)
    int source_index;                 // -1 = zone boundary, 0+ = obstacle index
} headland_section_t;

/**
 * Output of compute_headland().
 *
 * sections:            ordered list of headland path sections.
 * shrunken_zone:       zone boundary inset by (path_width/2 + headland_coverage_offset).
 * expanded_obstacles:  obstacle boundaries expanded by the same amount.
 */
typedef struct
{
    cvector_vector_type(headland_section_t) sections;
    polygon_t shrunken_zone;
    polygon_t *expanded_obstacles;
    uint32_t expanded_obstacle_count;
} headland_t;

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
 * geometry for subsequent coverage planning.
 *
 * Returns 0 on success, negative on error:
 *   -2  allocation failure
 *   -10 two or more expanded obstacles overlap each other
 *   -11 an expanded obstacle escapes the shrunken zone
 */
int compute_headland(const input_environment_t *env, headland_t *headland);

/**
 * Frees all memory owned by a headland_t. Safe to call on a zero-initialised
 * struct.
 */
void free_headland(headland_t *headland);

#endif // HEADLAND_H
