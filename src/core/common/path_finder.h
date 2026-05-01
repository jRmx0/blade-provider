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
 * environment's free space, defined by the boundary offset inward and all
 * obstacle offsets outward by `offset`.
 *
 * Returns a cvector of waypoints on success (caller must free with cvector_free).
 * Returns NULL when no path exists (A* exhausted or allocation failure).
 */
cvector_vector_type(point_t) find_free_space_path(
    point_t from, point_t to,
    const input_environment_t *env,
    float offset);

#endif // PATH_FINDER_H
