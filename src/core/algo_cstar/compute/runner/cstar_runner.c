#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "cstar_runner.h"
#include "core/rcg/cstar_rcg.h"
#include "core/preprocess/cstar_lap.h"
#include "core/sampling/cstar_sampling.h"
#include "core/rcg/cstar_rcg_growth.h"
#include "core/rcg/cstar_rcg_prunning.h"
#include "core/waypoint/cstar_waypoint.h"
#include "core/dead_end/cstar_dead_end.h"
#include "core/coverage_hole/cstar_coverage_hole.h"
#include "debug/cstar_debug.h"

#include "core/rcg/cstar_rcg.c"
#include "core/geometry/cstar_geometry.c"
#include "core/preprocess/cstar_lap.c"
#include "core/sampling/cstar_sampling.c"
#include "core/rcg/cstar_rcg_growth.c"
#include "core/rcg/cstar_rcg_prunning.c"
#include "core/waypoint/cstar_waypoint.c"
#include "core/dead_end/cstar_dead_end.c"
#include "core/coverage_hole/cstar_coverage_hole.c"
#include "debug/cstar_debug.c"

#include "utils/cstar_runner_math.c"
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

    // RCG graph expansion: connect frontier-sampled nodes into a planar graph
    if (!cstar_rcg_expand_graph(&rcg, env))
    {
        cstar_debug_dispose(&debug_state);
        cstar_rcg_free(&rcg);
        cstar_result_cleanup_partial(result);
        cstar_environment_laps_cleanup(env);
        return NULL;
    }

    // RCG pruning: keep only end nodes and their edges
    cstar_rcg_prune_non_essential_nodes(&rcg);
    cstar_rcg_generate_vertical_lap_edges(&rcg, env);
    // Sync node neighbor fields (neighbor_up/down/left/right) with the new
    // vertical edges added above. cstar_rcg_generate_vertical_lap_edges adds
    // edges to rcg->edges but does not update the in-node pointers, so
    // cstar_select_goal_node would see stale CSTAR_NO_NEIGHBOR links.
    cstar_rcg_rebuild_links_from_edges(rcg.nodes, rcg.node_count, rcg.edges, rcg.edge_count);

    if (!cstar_debug_export_laps(&debug_state, env) ||
        !cstar_debug_export_rcg_nodes(&debug_state, &rcg) ||
        !cstar_debug_export_rcg_edges(&debug_state, &rcg))
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

    // Export link nodes created during traversal, finalize debug layers, then
    // dispose. Non-fatal: coverage segments in result are the primary output.
    cstar_debug_export_link_nodes(&debug_state, &rcg);
    cstar_debug_finalize_layers(&debug_state, &result->debug_layers);
    cstar_debug_dispose(&debug_state);

    cstar_rcg_free(&rcg);
    cstar_environment_laps_cleanup(env);
    return result;
}
