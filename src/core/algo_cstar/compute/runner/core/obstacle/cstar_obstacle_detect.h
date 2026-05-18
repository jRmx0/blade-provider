#ifndef CSTAR_OBSTACLE_DETECT_H
#define CSTAR_OBSTACLE_DETECT_H

#include <stdbool.h>
#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Collision Detection
// -------------------------------------------------------------------------

/**
 * Probes the segment from→to at path_width intervals for intersection with
 * any operationalObstacle. On the first hit, finds the exact parametric entry
 * point via edge intersection and fills the output parameters.
 *
 * Parameters:
 *   from             - Segment start (robot's current position)
 *   to               - Segment end   (planned goal)
 *   w                - Path width (probe interval)
 *   env              - Environment with operationalObstacles
 *   entry_pt_out     - Exact point where the path first enters the obstacle
 *   obstacle_idx_out - Index into env->operationalObstacles of the hit obstacle
 *
 * Returns:
 *   true  - Collision detected; outputs filled
 *   false - Path is clear
 */
bool cstar_path_has_collision(point_t from,
                              point_t to,
                              float w,
                              const cstar_environment_t *env,
                              point_t *entry_pt_out,
                              int *obstacle_idx_out);

// -------------------------------------------------------------------------
// Boundary Following
// -------------------------------------------------------------------------

/**
 * Walks the CCW obstacle boundary (forward vertex index order, matching
 * POLYGON_WINDING_CCW) from entry_pt until a clear sight-line to destination
 * is found or the full perimeter is traversed.
 *
 * Collects waypoints at ≤ w spacing along each boundary edge.
 * The first waypoint in the returned vector is entry_pt itself.
 *
 * Parameters:
 *   entry_pt     - Exact point on the obstacle boundary where collision occurred
 *   destination  - The goal the robot was trying to reach
 *   obstacle_idx - Index into env->operationalObstacles
 *   w            - Path width (waypoint spacing and sight-line probe interval)
 *   env          - Environment
 *
 * Returns:
 *   cvector of point_t waypoints (caller must cvector_free); NULL on failure.
 */
cvector_vector_type(point_t) cstar_obstacle_follow_boundary(point_t entry_pt,
                                                            point_t destination,
                                                            int obstacle_idx,
                                                            float w,
                                                            const cstar_environment_t *env);

// -------------------------------------------------------------------------
// Full Circumnavigation
// -------------------------------------------------------------------------

/**
 * Walks the entire CCW obstacle boundary (forward vertex index order) starting
 * at entry_pt, visits every vertex, and closes the loop back to entry_pt.
 *
 * Unlike cstar_obstacle_follow_boundary, this function never exits early —
 * it always performs a full circumnavigation regardless of sight-lines.
 * Intermediate waypoints are inserted at ≤ w spacing along each edge.
 *
 * Parameters:
 *   entry_pt     - Exact point on the obstacle boundary where collision occurred
 *   obstacle_idx - Index into env->operationalObstacles
 *   w            - Path width (waypoint spacing)
 *   env          - Environment
 *
 * Returns:
 *   cvector of point_t waypoints starting and ending at entry_pt (caller must
 *   cvector_free); NULL on failure.
 */
cvector_vector_type(point_t) cstar_obstacle_circumnavigate(point_t entry_pt,
                                                           int obstacle_idx,
                                                           float w,
                                                           const cstar_environment_t *env);

// -------------------------------------------------------------------------
// RCG Update
// -------------------------------------------------------------------------

/**
 * Adds each waypoint as a new RCG node (is_link_node = true, lap_id = -1),
 * chains edges between consecutive nodes, connects the exit node to nearby
 * Open nodes within √2·w, and rebuilds neighbor pointers from the edge list.
 *
 * Parameters:
 *   rcg          - RCG to update
 *   waypoints    - Ordered boundary waypoints from cstar_obstacle_follow_boundary
 *   from_node_id - ID of the node at the current position (start of chain)
 *   w            - Path width
 *   env          - Environment
 *
 * Returns:
 *   Stable node ID of the exit (last injected) node, or CSTAR_NO_NEIGHBOR.
 */
int cstar_obstacle_inject_boundary_nodes(cstar_rcg_t *rcg,
                                         const cvector_vector_type(point_t) waypoints,
                                         int from_node_id,
                                         float w,
                                         const cstar_environment_t *env);

// -------------------------------------------------------------------------
// Obstacle-Adjacent Frontier Sampling
// -------------------------------------------------------------------------

/**
 * Adds new lap-based frontier samples to an existing RCG for all laps that
 * fall within sqrt(2)*w of the given obstacle's X extent.
 *
 * Unlike cstar_generate_frontier_samples, this function:
 *   - Does NOT clear any existing lap->node_ids.
 *   - Does NOT reset the RCG.
 *   - Only processes laps adjacent to the obstacle.
 *   - Skips positions already occupied by an existing RCG node (within epsilon).
 *
 * New nodes are connected to nearby existing nodes via unique edges and
 * neighbor pointers are rebuilt from the edge list.
 *
 * Parameters:
 *   rcg          - Existing RCG to extend
 *   obstacle_idx - Index into env->operationalObstacles of the hit obstacle
 *   w            - Path width (sample spacing and connection threshold)
 *   delta        - Frontier spacing multiplier (same as initial sampling)
 *   env          - Environment
 *
 * Returns:
 *   Number of new nodes added, or 0 if none.
 */
int cstar_generate_obstacle_adjacent_samples(cstar_rcg_t *rcg,
                                             int obstacle_idx,
                                             float w,
                                             int delta,
                                             const cstar_environment_t *env);

#endif // CSTAR_OBSTACLE_DETECT_H
