/**
 * path_finder.h
 *
 * Free-space path finding between two points given an environment.
 * Algorithm-agnostic; suitable for any module in src/core/.
 *
 * Uses a visibility-graph A* over offset polygon vertices to find the
 * shortest collision-free path.
 */

#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include "../core_types.h"
#include "../../../dependencies/cvector/cvector.h"

/*
 * Finds the shortest collision-free path from `from` to `to` within the
 * environment's free space (env->boundary as the navigable zone,
 * env->obstacles as exclusion zones).
 *
 * Returns a cvector of waypoints on success (caller must free with cvector_free).
 * Returns NULL when no path exists (A* exhausted or allocation failure).
 */
cvector_vector_type(point_t) find_free_space_path(
    point_t from, point_t to,
    const input_environment_t *env);

/*
 * Same as find_free_space_path but injects additional relay points into the
 * visibility graph node set.  This allows A* to route through known
 * waypoints (e.g. coverage section start/end points) that would otherwise
 * be invisible to the planner.
 *
 * extra_nodes  - array of extra points to add as graph nodes (may be NULL)
 * extra_count  - number of entries in extra_nodes (0 = no extras)
 */
cvector_vector_type(point_t) find_free_space_path_ex(
    point_t from, point_t to,
    const input_environment_t *env,
    const point_t *extra_nodes, int extra_count);

#endif // PATH_FINDER_H
