#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../../dependencies/allocator/allocator.h"
#include "cstar_runner.h"
#include "core/rcg/cstar_rcg.h"
#include "core/preprocess/cstar_lap.h"
#include "core/sampling/cstar_sampling.h"
#include "core/rcg/cstar_rcg_growth.h"
#include "core/rcg/cstar_rcg_prunning.h"
#include "core/waypoint/cstar_waypoint.h"
#include "core/dead_end/cstar_dead_end.h"
#include "debug/cstar_debug.h"
#include "utils/cstar_runner_math.h"

#include "utils/cstar_runner_math.c"
#include "core/rcg/cstar_rcg.c"
#include "core/geometry/cstar_geometry.c"
#include "core/preprocess/cstar_lap.c"
#include "core/sampling/cstar_sampling.c"
#include "core/rcg/cstar_rcg_growth.c"
#include "core/rcg/cstar_rcg_prunning.c"
#include "core/waypoint/cstar_waypoint.c"
#include "core/dead_end/cstar_dead_end.c"
#include "core/obstacle/cstar_obstacle_detect.c"
#include "debug/cstar_debug.c"

#include "utils/cstar_runner_result.c"
#include "utils/cstar_runner_segments.c"

cstar_coverage_path_result_t *cstar_coverage_path_planning_process(cstar_environment_t *env)
{
    if (env == NULL)
    {
        return NULL;
    }

    if (env->operationalBoundary.vertex_count < 3u || env->operationalBoundary.vertices == NULL)
    {
        return NULL;
    }

    cstar_rcg_t rcg;
    cstar_rcg_init(&rcg);
    cstar_debug_t debug_state = {0};

    cstar_coverage_path_result_t *result = cstar_result_create();
    if (result == NULL)
    {
        cstar_rcg_free(&rcg);
        return NULL;
    }

    if (!cstar_debug_init(&debug_state))
    {
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        return NULL;
    }

    float w = env->path_width;

    va_tracking_mark("Praėjimai");
    // One-time preprocessing: generate and store laps in environment
    if (!cstar_preprocess_environment_laps(env, w))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    int delta = (env->frontier_spacing_multiplier > 0u)
                    ? (int)env->frontier_spacing_multiplier
                    : 1;

    va_tracking_mark("Ribiniai taškai");
    int generated_samples = cstar_generate_frontier_samples(&rcg,
                                                            w,
                                                            delta,
                                                            env);
    if (generated_samples <= 0)
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    va_tracking_mark("RCG");
    // RCG graph expansion: connect frontier-sampled nodes into a planar graph
    if (!cstar_rcg_expand_graph(&rcg, env))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    // RCG pruning: keep only end nodes and their edges; then do a full graph
    // update — flush all edges, re-derive cross-lap and same-lap connectivity
    // for the surviving node set, and sync all in-node neighbor pointers.
    va_tracking_mark("Atšakų šalinimas");
    cstar_rcg_prune_non_essential_nodes(&rcg);
    cstar_rcg_full_graph_update(&rcg, env);

    if (!cstar_debug_export_laps(&debug_state, env) ||
        !cstar_debug_export_rcg_nodes(&debug_state, &rcg))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    // debug_state stays alive — link nodes are created during the main loop
    // and captured by cstar_debug_export_link_nodes after the loop exits.

    // -------------------------------------------------------------------------
    // Coverage path planning loop  (Section III.B, Algorithms 1–2 + IV)
    // -------------------------------------------------------------------------

    va_tracking_mark("Planavimas");
    // Locate the start node placed at env->start_point during sampling.
    int start_node_id = CSTAR_NO_NEIGHBOR;
    for (int i = 0; i < rcg.node_count; ++i)
    {
        if (rcg.nodes[i].is_start_point)
        {
            start_node_id = rcg.nodes[i].id;
            break;
        }
    }

    if (start_node_id == CSTAR_NO_NEIGHBOR)
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_environment_laps_cleanup(env);
        cstar_result_cleanup_partial(result);
        return NULL;
    }

    int segment_id = 0;

    // Emit an initial transit segment when start_point does not coincide
    // exactly with the start RCG node (floating-point guard; usually skipped).
    const cstar_node_t *start_node = cstar_rcg_get_node_by_id(&rcg, start_node_id);
    if (start_node != NULL &&
        !cstar_points_equal(env->start_point, start_node->pos))
    {
        if (!cstar_add_start_transit(result, &segment_id,
                                     env->start_point, start_node->pos))
        {
            cstar_debug_dispose(&debug_state);
            cstar_rcg_free(&rcg);
            cstar_environment_laps_cleanup(env);
            cstar_result_cleanup_partial(result);
            return NULL;
        }
    }

    int current_node_id = start_node_id;
    cvector_vector_type(int) retreat_nodes = NULL;
    bool first_coverage_move_done = false;

    for (;;)
    {
        // Re-fetch every iteration: cstar_update_node_state may reallocate nodes.
        const cstar_node_t *cur = cstar_rcg_get_node_by_id(&rcg, current_node_id);
        if (cur == NULL)
        {
            break;
        }

        // Keep the retreat node set current at the robot's position.
        cstar_retreat_update(&retreat_nodes, &rcg, cur->pos, w);

        // Select next goal: left → up → down → right priority.
        int goal_id = cstar_select_goal_node(&rcg, current_node_id);

        if (goal_id == CSTAR_NO_NEIGHBOR)
        {
            // Close the dead-end node so it is excluded from the retreat set
            // before escape. No goal was reached so cstar_update_node_state
            // was never called — close the node directly here instead.
            cstar_node_t *dead_end_node = cstar_rcg_get_node_by_id_mut(&rcg, current_node_id);
            if (dead_end_node != NULL)
            {
                dead_end_node->state = CSTAR_NODE_CL;
            }

            // Refresh: removes the now-closed dead-end node and reflects the
            // final Open neighbours around this position.
            cstar_retreat_update(&retreat_nodes, &rcg, cur->pos, w);

            // Navigate to nearest retreat node via A*.
            cvector_vector_type(point_t) escape_path = NULL;
            int retreat_id = cstar_escape_dead_end(&rcg,
                                                   current_node_id,
                                                   retreat_nodes,
                                                   &escape_path);
            if (retreat_id == CSTAR_NO_NEIGHBOR)
            {
                // Retreat set empty — every reachable node is Closed.
                // Coverage is complete.
                if (escape_path != NULL)
                {
                    cvector_free(escape_path);
                }
                break;
            }

            // Emit the A* escape path as a retreatTransit segment.
            if (escape_path != NULL)
            {
                int path_len = (int)cvector_size(escape_path);
                if (path_len > 0)
                {
                    cstar_result_add_segment(result, segment_id,
                                             "retreatTransit",
                                             escape_path, path_len);
                    segment_id++;
                }
                cvector_free(escape_path);
            }

            current_node_id = retreat_id;
            continue;
        }

        // Capture positions before state update may reallocate rcg->nodes.
        point_t from_pos = cur->pos;
        const cstar_node_t *goal_node = cstar_rcg_get_node_by_id(&rcg, goal_id);
        if (goal_node == NULL)
        {
            break;
        }
        point_t to_pos = goal_node->pos;

        // ── Runtime obstacle collision detection ──────────────────────────────────
        // On collision:
        //   1. Emit approach segment (from_pos → entry_pt).
        //   2. Emit full CCW circumnavigation back to entry_pt.
        //   3. Add new lap-based frontier samples on obstacle-adjacent laps.
        //   4. Prune new samples, generate vertical lap edges, rebuild links.
        {
            point_t entry_pt;
            int hit_obstacle_idx = -1;
            if (cstar_path_has_collision(from_pos, to_pos, w, env,
                                         &entry_pt, &hit_obstacle_idx))
            {
                const char *seg_type = first_coverage_move_done
                                           ? "coverage"
                                           : "coverageTransit";

                /* Segment 1: last RCG node → collision point. */
                point_t approach[2] = {from_pos, entry_pt};
                cstar_result_add_segment(result, segment_id, seg_type,
                                         approach, 2);
                segment_id++;
                first_coverage_move_done = true;

                /* Segment 2: full CCW circumnavigation, closing back to entry_pt. */
                cvector_vector_type(point_t) boundary_pts =
                    cstar_obstacle_circumnavigate(entry_pt, hit_obstacle_idx,
                                                  w, env);
                if (boundary_pts != NULL)
                {
                    int bp_count = (int)cvector_size(boundary_pts);
                    if (bp_count > 0)
                    {
                        cstar_result_add_segment(result, segment_id, "coverage",
                                                 boundary_pts, bp_count);
                        segment_id++;
                    }
                    cvector_free(boundary_pts);
                }

                /* Capture the next-to-be-assigned node ID before sampling.
                   New obstacle-adjacent nodes will receive IDs >= this value.
                   Using the ID boundary (not the array index) is stable across
                   the second pruning pass: link nodes added during the coverage
                   loop may be pruned there, shifting the compact indices of new
                   nodes below nodes_before — the ID boundary is unaffected. */
                int new_node_id_threshold = rcg.next_node_id;
                cstar_generate_obstacle_adjacent_samples(&rcg, hit_obstacle_idx,
                                                         w, delta, env);

                /* Prune new nodes to end-nodes-only, then do a full graph
                   update: flush all edges, re-derive cross-lap and same-lap
                   connectivity for the current node set (including newly
                   inserted obstacle-adjacent nodes), and sync all in-node
                   neighbor pointers.  Mirrors the initial RCG setup sequence. */
                cstar_rcg_prune_non_essential_nodes(&rcg);
                cstar_rcg_full_graph_update(&rcg, env);

                /* Append only newly-added survived nodes to the debug layer.
                   Filtered by node ID (>= new_node_id_threshold) so pre-existing
                   nodes are not duplicated regardless of how compaction reorders
                   the array. */
                cstar_debug_export_rcg_nodes_from_id(&debug_state, &rcg,
                                                     new_node_id_threshold);

                /* Close current_node_id and insert link nodes as if the robot
                   completed the move to goal_id.  The departure from
                   current_node_id is real (the robot moved toward entry_pt),
                   so the same can_close rule and left-lap link-node logic
                   apply here exactly as in the normal post-move update. */
                cstar_update_node_state(&rcg, current_node_id, goal_id, w);

                /* Close any Open link node whose position is within w of the
                   hit obstacle boundary.  Such a node sits in the obstacle's
                   danger zone: any cross-lap approach to it triggers a
                   collision, which spawns more link nodes in the same zone,
                   trapping the algorithm in an infinite cascade.
                   Link nodes that are farther than w from the obstacle are
                   left Open so they can still cover the areas above and below
                   the obstacle. The obstacle-adjacent nodes generated above
                   handle coverage inside the w-wide corridor around the
                   obstacle itself. */
                {
                    const polygon_t *hit_obs =
                        &env->operationalObstacles[hit_obstacle_idx];
                    uint32_t edge_count = hit_obs->vertex_count;

                    for (int k = 0; k < rcg.node_count; ++k)
                    {
                        cstar_node_t *ln = &rcg.nodes[k];
                        if (!ln->is_link_node || ln->state == CSTAR_NODE_CL)
                            continue;

                        float min_dist;
                        if (cstar_sampling_point_in_polygon(ln->pos, hit_obs))
                        {
                            min_dist = 0.0f;
                        }
                        else
                        {
                            min_dist = INFINITY;
                            for (uint32_t ei = 0; ei < edge_count; ++ei)
                            {
                                point_t a = hit_obs->vertices[ei];
                                point_t b = hit_obs->vertices[(ei + 1u) % edge_count];
                                float d = cstar_sampling_dist_point_segment(
                                    ln->pos, a, b);
                                if (d < min_dist)
                                    min_dist = d;
                            }
                        }

                        if (min_dist < w)
                            ln->state = CSTAR_NODE_CL;
                    }
                }

                /* Resume coverage from the nearest Open RCG node to entry_pt.
                   The robot completed a full CCW circumnavigation and is back at
                   entry_pt; we must advance current_node_id so the loop restarts
                   from the correct physical position rather than replaying the
                   collision endlessly. */
                {
                    int resume_id = CSTAR_NO_NEIGHBOR;
                    float resume_dist = INFINITY;
                    for (int k = 0; k < rcg.node_count; ++k)
                    {
                        const cstar_node_t *rn = &rcg.nodes[k];
                        if (rn->state == CSTAR_NODE_CL ||
                            rn->is_start_point ||
                            rn->is_link_node ||
                            rn->id == current_node_id)
                            continue; /* never resume at the collision source */
                        float ddx = rn->pos.x - entry_pt.x;
                        float ddy = rn->pos.y - entry_pt.y;
                        float d = sqrtf(ddx * ddx + ddy * ddy);
                        if (d < resume_dist)
                        {
                            resume_dist = d;
                            resume_id = rn->id;
                        }
                    }

                    if (resume_id == CSTAR_NO_NEIGHBOR)
                        break; /* all nodes closed; coverage complete */

                    /* Emit transit: circumnavigation close point → resume node.
                       Bridges the physical gap between entry_pt and the nearest
                       planning node so the output path is continuous. */
                    const cstar_node_t *rnode = cstar_rcg_get_node_by_id(&rcg, resume_id);
                    if (rnode != NULL && !cstar_points_equal(entry_pt, rnode->pos))
                    {
                        point_t transit[2] = {entry_pt, rnode->pos};
                        cstar_result_add_segment(result, segment_id,
                                                 "coverage", transit, 2);
                        segment_id++;
                    }

                    current_node_id = resume_id;
                }
                continue; /* restart coverage loop from resume node */
            }
        }

        // Emit segment: current → goal. The very first move from the start
        // node is a coverageTransit (positioning to first coverage line);
        // all subsequent moves are true coverage segments.
        const char *seg_type = first_coverage_move_done ? "coverage" : "coverageTransit";
        point_t seg_path[2] = {from_pos, to_pos};
        if (!cstar_result_add_segment(result, segment_id, seg_type,
                                      seg_path, 2))
        {
            break;
        }
        segment_id++;
        first_coverage_move_done = true;

        // Close current node; insert link nodes on left-lap transitions.
        cstar_update_node_state(&rcg, current_node_id, goal_id, w);

        current_node_id = goal_id;
    }

    // Export the final retreat node set (snapshot at loop exit) then free.
    cstar_debug_accumulate_retreat_nodes(&debug_state, retreat_nodes, &rcg);
    cvector_free(retreat_nodes);

    // Export link nodes created during traversal and the final RCG edge set
    // (after all mid-loop modifications), finalize debug layers, then dispose.
    // Non-fatal: coverage segments in result are the primary output.
    cstar_debug_export_link_nodes(&debug_state, &rcg);
    cstar_debug_export_rcg_edges(&debug_state, &rcg);
    cstar_debug_finalize_layers(&debug_state, &result->debug_layers);
    cstar_debug_dispose(&debug_state);

    va_tracking_mark("Valymas");
    cstar_rcg_free(&rcg);
    cstar_environment_laps_cleanup(env);
    return result;
}
