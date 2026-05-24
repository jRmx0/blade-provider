#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "bcd_runner.h"
#include "../../../common/clog.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"
#include "../../../../../dependencies/allocator/allocator.h"
#include "core/event_list/bcd_event_list_building.h"
#include "core/cells/bcd_cell_computation.h"
#include "core/coverage_planning/bcd_coverage_planning.h"
#include "core/motion_planning/bcd_motion_planning.h"
#include "core/geometry/bcd_geometry.h"
#include "core/pathfinding/bcd_pathfinding.h"
#include "core/ox_motion/bcd_ox_motion.h"
#include "core/preprocess/bcd_preprocess.h"
#include "../../../common/headland.h"
#include "../../../common/path_finder.h"

#include "core/event_list/bcd_event_list_building.c"
#include "core/cells/bcd_cell_computation.c"
#include "core/pathfinding/bcd_pq.c"
#include "core/coverage_planning/bcd_coverage_planning.c"
#include "core/geometry/bcd_geometry.c"
#include "core/pathfinding/bcd_pathfinding.c"
#include "core/ox_motion/bcd_ox_motion.c"
#include "core/motion_planning/bcd_motion_planning.c"
#include "core/preprocess/bcd_preprocess.c"
#include "../../../common/path_finder.c"
#include "../../../common/headland.c"
#include "utils/bcd_runner_result.c"

#include "../../serialize/bcd_serialize.c"

bcd_result_t *coverage_path_planning_process(input_environment_t *env, bcd_compute_error_t *err_out)
{
	bcd_event_list_t event_list;
	event_list.bcd_events = NULL;
	event_list.length = 0;

	// --- Headland pass disabled ---
	// When env->headland is true, BCD skips the perimeter headland coverage pass
	// and performs only interior cell coverage, stopping at the last coverage point.
	headland_t headland;
	memset(&headland, 0, sizeof(headland_t));
	bool has_headland = false;

	input_environment_t *active_env = env;

	va_tracking_mark("Paruo┼íimas");
	int rc = bcd_preprocess_environment(active_env, 0.0f);
	if (rc != 0)
	{
		LOG_ERROR("coverage_path_planning: environment preprocessing failed (code %d)", rc);
		if (has_headland)
			free_headland(&headland);
		if (err_out)
		{
			err_out->code = "BCD_PREPROCESS_FAILED";
			err_out->message = "BCD environment preprocessing failed";
		}
		return NULL;
	}

	/* DEBUG: dump all vertex X values after preprocessing to verify uniqueness */
	{
		LOG_DEBUG("BCD preprocess: boundary vertices (%u):", active_env->boundary.vertex_count);
		for (uint32_t _vi = 0; _vi < active_env->boundary.vertex_count; _vi++)
			LOG_DEBUG("  [%u] x=%.9f  y=%.9f", _vi,
					  active_env->boundary.vertices[_vi].x,
					  active_env->boundary.vertices[_vi].y);
		for (uint32_t _oi = 0; _oi < active_env->obstacle_count; _oi++)
		{
			LOG_DEBUG("BCD preprocess: obstacle[%u] vertices (%u):",
					  _oi, active_env->obstacles[_oi].vertex_count);
			for (uint32_t _vi = 0; _vi < active_env->obstacles[_oi].vertex_count; _vi++)
				LOG_DEBUG("  [%u] x=%.9f  y=%.9f", _vi,
						  active_env->obstacles[_oi].vertices[_vi].x,
						  active_env->obstacles[_oi].vertices[_vi].y);
		}
	}

	va_tracking_mark("─«vykiai");
	rc = build_bcd_event_list(active_env, &event_list);
	if (rc != 0)
	{
		LOG_ERROR("coverage_path_planning: BCD event list generation failed (code %d)", rc);
		if (has_headland)
			free_headland(&headland);
		if (err_out)
		{
			err_out->code = "BCD_EVENT_LIST_FAILED";
			err_out->message = "BCD event list generation failed";
		}
		return NULL;
	}
	LOG_DEBUG("coverage_path_planning: successfully generated %d events", event_list.length);
	for (int ei = 0; ei < event_list.length; ei++)
	{
		bcd_event_t *ev = &event_list.bcd_events[ei];
		LOG_DEBUG("  [%d] type=%-10s vertex=(%.9f, %.9f)",
				  ei,
				  event_type_to_string(ev->bcd_event_type),
				  ev->polygon_vertex.x,
				  ev->polygon_vertex.y);
	}
	cvector_vector_type(bcd_cell_t) cell_list = NULL;
	va_tracking_mark("L─àstel─ùs");
	rc = compute_bcd_cells(&event_list, &cell_list);
	if (rc != 0)
	{
		LOG_ERROR("coverage_path_planning: BCD cell computation failed (code %d)", rc);
		if (has_headland)
			free_headland(&headland);
		if (err_out)
		{
			err_out->code = "BCD_CELL_COMPUTATION_FAILED";
			err_out->message = "BCD cell computation failed";
		}
		return NULL;
	}
	LOG_DEBUG("coverage_path_planning: successfully generated %zu cells", cvector_size(cell_list));
	// log_bcd_cell_list((const cvector_vector_type(bcd_cell_t) *) &cell_list);

	cvector_vector_type(int) path_list = NULL;
	va_tracking_mark("L─àsteli┼│ seka");
	int starting_cell_index = bcd_find_starting_cell(
		(const cvector_vector_type(bcd_cell_t) *)&cell_list,
		active_env->start_point);
	rc = compute_bcd_path_list(&cell_list, starting_cell_index, &path_list);
	if (rc != 0)
	{
		LOG_ERROR("coverage_path_planning: BCD path computation failed (code %d)", rc);
		if (has_headland)
			free_headland(&headland);
		if (err_out)
		{
			err_out->code = "BCD_PATH_LIST_FAILED";
			err_out->message = "BCD path list computation failed";
		}
		return NULL;
	}
	LOG_DEBUG("coverage_path_planning: successfully generated path with %zu visits", cvector_size(path_list));
	// log_bcd_path_list((const cvector_vector_type(int) *)&path_list);

	bcd_motion_plan_t motion_plan = {0};
	va_tracking_mark("Padengimo kelias");
	rc = compute_bcd_motion(&cell_list,
							(const cvector_vector_type(int) *)&path_list,
							&motion_plan,
							active_env->path_width - active_env->path_overlap,
							active_env->end_point);
	if (rc != 0)
	{
		LOG_ERROR("coverage_path_planning: BCD motion computation failed (code %d)", rc);
		if (has_headland)
			free_headland(&headland);
		if (err_out)
		{
			err_out->code = "BCD_MOTION_FAILED";
			err_out->message = "BCD motion planning failed";
		}
		return NULL;
	}
	log_bcd_motion(motion_plan);

	va_tracking_mark("Mar┼írutas");
	// --- Build one visibility graph for ALL transit segments -----------
	//
	// All coverage section start/end points, headland section start/end points,
	// and the global start/end points are now known.  Inject every one of them
	// as extra nodes so the single O(n┬▓) edge-validity sweep covers them all.
	// Subsequent A* queries just read the pre-computed adjacency matrix.
	//
	// Transit segments that need paths:
	//   (a) coverage section[i] end ΓåÆ coverage section[i+1] start
	//   (b) coverage section[last] end ΓåÆ env->end_point  (only when env->headland == false)
	//   (c) headland section[last] end ΓåÆ coverage section[0] start  (unused: headland pass is disabled)
	//   (d) env->start_point ΓåÆ coverage section[0] start

	cvector_vector_type(point_t) all_transit_nodes = NULL;

	// Always include global start.
	cvector_push_back(all_transit_nodes, env->start_point);
	// Include end_point only when headland is off ΓÇö when headland is on, coverage
	// stops at the last coverage point and end_point transit is skipped.
	if (!env->headland)
		cvector_push_back(all_transit_nodes, active_env->end_point);

	// Headland section endpoints (unused: headland pass is disabled).
	if (has_headland && headland.sections != NULL)
	{
		int hl_n = (int)cvector_size(headland.sections);
		for (int hi = 0; hi < hl_n; ++hi)
		{
			const headland_section_t *hs = &headland.sections[hi];
			if (hs->path == NULL || cvector_size(hs->path) == 0)
				continue;
			cvector_push_back(all_transit_nodes, hs->path[0]);
			point_t ep = hs->path[cvector_size(hs->path) - 1];
			cvector_push_back(all_transit_nodes, ep);
		}
	}

	// Coverage section endpoints.
	if (motion_plan.section != NULL)
	{
		int sc = (int)cvector_size(motion_plan.section);
		for (int ri = 0; ri < sc; ++ri)
		{
			const cell_motion_plan_t *rs = &motion_plan.section[ri];
			if (rs->ox == NULL || cvector_size(rs->ox) == 0)
				continue;
			point_t sp = rs->ox[0];
			point_t ep = rs->ox[cvector_size(rs->ox) - 1];
			cvector_push_back(all_transit_nodes, sp);
			if (ep.x != sp.x || ep.y != sp.y)
				cvector_push_back(all_transit_nodes, ep);
		}
	}

	// Build the graph once.
	vg_graph_t *vg = vg_graph_build(
		env,
		all_transit_nodes != NULL ? all_transit_nodes : NULL,
		all_transit_nodes != NULL ? (int)cvector_size(all_transit_nodes) : 0);
	cvector_free(all_transit_nodes);

	if (vg == NULL)
	{
		if (has_headland)
			free_headland(&headland);
		free_bcd_event_list(&event_list);
		free_bcd_cell_list(&cell_list);
		cvector_free(path_list);
		free_bcd_motion(&motion_plan);
		if (err_out)
		{
			err_out->code = "vg_build_failed";
			err_out->message = "Visibility graph construction failed (allocation error).";
		}
		return NULL;
	}

	// --- (a)+(b) Coverage transit segments ---
	if (motion_plan.section != NULL && cvector_size(motion_plan.section) > 0)
	{
		int section_count = (int)cvector_size(motion_plan.section);
		for (int i = 0; i < section_count; ++i)
		{
			cell_motion_plan_t *curr = &motion_plan.section[i];
			if (curr->ox == NULL || cvector_size(curr->ox) == 0)
				continue;

			point_t from_pt = curr->ox[cvector_size(curr->ox) - 1];
			point_t to_pt = {0};
			bool has_target = false;

			if (i + 1 < section_count)
			{
				cell_motion_plan_t *next = &motion_plan.section[i + 1];
				if (next->ox != NULL && cvector_size(next->ox) > 0)
				{
					to_pt = next->ox[0];
					has_target = true;
				}
			}
			else if (!env->headland)
			{
				// headland=OFF: navigate from last coverage point to end_point.
				to_pt = active_env->end_point;
				has_target = true;
			}
			// else: headland=ON ΓÇö stop at last coverage point, nav stays NULL.

			if (!has_target)
			{
				cvector_free(curr->nav);
				curr->nav = NULL;
				continue;
			}

			cvector_vector_type(point_t) nav = vg_graph_query(vg, from_pt, to_pt);
			if (nav == NULL)
			{
				vg_graph_free(vg);
				if (has_headland)
					free_headland(&headland);
				free_bcd_event_list(&event_list);
				free_bcd_cell_list(&cell_list);
				cvector_free(path_list);
				free_bcd_motion(&motion_plan);
				if (err_out)
				{
					err_out->code = "no_coverage_transit_path";
					err_out->message = "No collision-free path could be found for a coverage transit segment.";
				}
				return NULL;
			}

			cvector_free(curr->nav);
			curr->nav = nav;
		}
	}

	// --- (c) Headland section[last] end ΓåÆ coverage section[0] start ---
	if (has_headland && headland.sections != NULL)
	{
		int hl_count = (int)cvector_size(headland.sections);
		if (hl_count > 0 &&
			motion_plan.section != NULL && cvector_size(motion_plan.section) > 0 &&
			motion_plan.section[0].ox != NULL && cvector_size(motion_plan.section[0].ox) > 0)
		{
			headland_section_t *last_hs = &headland.sections[hl_count - 1];
			if (last_hs->path != NULL && cvector_size(last_hs->path) > 0)
			{
				point_t from_pt = last_hs->path[cvector_size(last_hs->path) - 1];
				point_t to_pt = motion_plan.section[0].ox[0];

				cvector_free(last_hs->nav);
				last_hs->nav = vg_graph_query(vg, from_pt, to_pt);
				if (last_hs->nav == NULL)
				{
					vg_graph_free(vg);
					free_headland(&headland);
					if (err_out)
					{
						err_out->code = "BCD_HEADLAND_NAV_FAILED";
						err_out->message = "BCD headland navigation path failed";
					}
					return NULL;
				}
			}
		}
	}

	// --- (d) Start point ΓåÆ first headland or coverage waypoint ---
	cvector_vector_type(point_t) start_nav = NULL;

	if (has_headland)
	{
		if (headland.sections != NULL && cvector_size(headland.sections) > 0)
		{
			headland_section_t *first_hs = &headland.sections[0];
			if (first_hs->path != NULL && cvector_size(first_hs->path) > 0)
			{
				point_t sp_from = env->start_point;
				point_t sp_to = first_hs->path[0];

				start_nav = vg_graph_query(vg, sp_from, sp_to);
				if (start_nav == NULL)
				{
					vg_graph_free(vg);
					free_headland(&headland);
					if (err_out)
					{
						err_out->code = "BCD_HEADLAND_START_NAV_FAILED";
						err_out->message = "BCD headland start navigation path failed";
					}
					return NULL;
				}
			}
		}
	}
	else
	{
		if (motion_plan.section != NULL && cvector_size(motion_plan.section) > 0 &&
			motion_plan.section[0].ox != NULL && cvector_size(motion_plan.section[0].ox) > 0)
		{
			point_t sp_from = active_env->start_point;
			point_t sp_to = motion_plan.section[0].ox[0];

			start_nav = vg_graph_query(vg, sp_from, sp_to);
			if (start_nav == NULL)
			{
				vg_graph_free(vg);
				free_bcd_event_list(&event_list);
				free_bcd_cell_list(&cell_list);
				cvector_free(path_list);
				free_bcd_motion(&motion_plan);
				if (err_out)
				{
					err_out->code = "no_start_transit_path";
					err_out->message = "No collision-free path could be found from the start point to the first coverage waypoint.";
				}
				return NULL;
			}
		}
	}

	/* vg served its purpose ΓÇö all nav paths are computed and stored in
	 * motion_plan / headland / start_nav. Free it before packaging. */
	vg_graph_free(vg);

	/* Package results for the orchestrator ΓÇö serialization and cleanup happen
	 * after tracking is disabled in bcd_run_compute (mirrors the C* pattern). */
	bcd_result_t *result = bcd_result_create();
	if (result == NULL)
	{
		if (has_headland)
			free_headland(&headland);
		free_bcd_event_list(&event_list);
		free_bcd_cell_list(&cell_list);
		cvector_free(path_list);
		free_bcd_motion(&motion_plan);
		cvector_free(start_nav);
		if (err_out)
		{
			err_out->code = "OOM";
			err_out->message = "Out of memory allocating BCD result";
		}
		return NULL;
	}

	bcd_result_populate(result, event_list, cell_list, path_list, motion_plan,
						has_headland, headland, start_nav);

	return result;
}

// POINT_T helpers

bool are_equal_points(point_t a,
					  point_t b)
{
	const float eps = 1e-4f;
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps;
}
