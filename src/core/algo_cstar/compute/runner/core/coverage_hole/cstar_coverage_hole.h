#ifndef CSTAR_COVERAGE_HOLE_H
#define CSTAR_COVERAGE_HOLE_H

#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Detection
// -------------------------------------------------------------------------

/**
 * Detects coverage holes adjacent to current_node_id (Definition I.1).
 *
 * A coverage hole is a contiguous set of Open nodes that is fully enclosed
 * by Closed nodes and obstacles, with no node touching the unknown area.
 *
 * Uses flood-fill starting from each unlabelled Open neighbour of
 * current_node_id. The goal node goal_node_id acts as a stop boundary.
 *
 * Returns a cvector where each element is a cvector(int) of node indices
 * belonging to one hole. Returns an empty outer vector when no holes exist.
 *
 * The caller is responsible for cvector_free()-ing every inner vector and
 * then the outer vector.
 */
cvector_vector_type(cvector_vector_type(int))
    cstar_detect_coverage_holes(const cstar_rcg_t *rcg,
                                int current_node_id,
                                int goal_node_id,
                                float w,
                                const cstar_environment_t *env);

// -------------------------------------------------------------------------
// TSP trajectory  (Algorithm 3)
// -------------------------------------------------------------------------

/**
 * Densifies the coverage hole with additional nodes (inter-node spacing w),
 * sets up the TSP, and returns the optimised visitation order as a
 * cvector(int) of node indices.
 *
 * The start node is always current_node_id. The end node is determined by
 * Algorithm 3 (goal node or current node, depending on their Open neighbours).
 *
 * Returns NULL if the hole is empty. The caller must cvector_free() the result.
 */
cvector_vector_type(int)
    cstar_compute_tsp_trajectory(cstar_rcg_t *rcg,
                                 int current_node_id,
                                 int goal_node_id,
                                 const int *hole_nodes,
                                 int hole_count,
                                 float w,
                                 const cstar_environment_t *env);

// -------------------------------------------------------------------------
// TSP solvers
// -------------------------------------------------------------------------

/**
 * Nearest-neighbour heuristic: builds an initial TSP tour starting from
 * start_id over the nodes in node_ids[0..count-1].
 *
 * Returns a cvector(int) tour; the caller must cvector_free() it.
 */
cvector_vector_type(int) cstar_tsp_nearest_neighbour(const int *node_ids,
                                                     int count,
                                                     int start_id,
                                                     const cstar_rcg_t *rcg);

/**
 * 2-opt improvement pass: repeatedly reverses sub-tours while the total cost
 * decreases. Mutates tour_nodes[0..count-1] in-place.
 */
void cstar_tsp_2opt(int *tour_nodes, int count, const cstar_rcg_t *rcg);

#endif // CSTAR_COVERAGE_HOLE_H
