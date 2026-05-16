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

#include <stdbool.h>
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

/* -----------------------------------------------------------------------
 * Pre-built visibility graph — build once, query many times.
 *
 * When all transit endpoints are known upfront (after cell computation and
 * coverage-path generation), build the graph once with every node injected.
 * The O(n²) edge-validity sweep runs once; each subsequent A* query is
 * cheap because it reads the pre-computed adjacency matrix instead of
 * re-evaluating segment visibility.
 *
 * Typical usage in bcd_runner:
 *
 *   vg_graph_t *vg = vg_graph_build(env, all_endpoints, endpoint_count);
 *   // for each transit:
 *   cvector_vector_type(point_t) path = vg_graph_query(vg, from, to);
 *   vg_graph_free(vg);
 * ----------------------------------------------------------------------- */

typedef struct
{
    cvector_vector_type(point_t) nodes; /* ordered list of all graph nodes  */
    bool *adj;                          /* n×n adjacency: adj[i*n+j]=free   */
    int n;                              /* total node count                  */
} vg_graph_t;

/*
 * Builds the visibility graph from the environment polygons plus the given
 * extra nodes (transit endpoints, start/end points, headland waypoints …).
 *
 * Node set: {extra_nodes} ∪ {all boundary vertices} ∪ {all obstacle vertices}.
 * All O(n²) segment visibility checks are evaluated at build time.
 *
 * Returns NULL on allocation failure.
 */
vg_graph_t *vg_graph_build(const input_environment_t *env,
                           const point_t *extra_nodes, int extra_count);

/*
 * Runs A* on the pre-built graph from `from` to `to`.
 * Both points are located in the node list by epsilon match.
 * Returns a cvector of waypoints (caller must cvector_free) or NULL.
 */
cvector_vector_type(point_t) vg_graph_query(const vg_graph_t *graph,
                                            point_t from, point_t to);

/*
 * Frees all memory owned by the graph.  Safe to call with NULL.
 */
void vg_graph_free(vg_graph_t *graph);

#endif // PATH_FINDER_H
