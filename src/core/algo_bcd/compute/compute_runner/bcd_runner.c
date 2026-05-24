#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "bcd_runner.h"
#include "../../../common/clog.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"
#include "../../../../../dependencies/allocator/allocator.h"
#include "bcd_core/bcd_event_list_building.h"
#include "bcd_core/bcd_cell_computation.h"
#include "bcd_core/bcd_coverage_planning.h"
#include "bcd_core/bcd_motion_planning.h"
#include "bcd_core/bcd_geometry.h"
#include "../../../common/headland.h"
#include "../../../common/path_finder.h"
#include "../../preprocess/bcd_preprocess.h"

#include "../../../common/path_finder.c"
#include "../../../common/headland.c"
#include "bcd_runner_result.c"

static void log_event_list(const bcd_event_list_t *event_list);
static const char *event_type_to_string(bcd_event_type_t t);
static const char *polygon_type_to_string(polygon_type_t t);
static char *serialize_event_list_json(const bcd_event_list_t *event_list);

cJSON *bcd_build_result_json_tree(const bcd_result_t *result)
{
	/* Local aliases — the rest of the function body is unchanged from the old
	 * serialize_result_json so all existing field accesses stay valid. */
	const bcd_event_list_t *event_list = &result->event_list;
	cvector_vector_type(bcd_cell_t) const *cell_list = &result->cell_list;
	cvector_vector_type(int) const *path_list = &result->path_list;
	const bcd_motion_plan_t *motion_plan = &result->motion_plan;
	const headland_t *headland = result->has_headland ? &result->headland : NULL;
	cvector_vector_type(point_t) start_nav = result->start_nav;

	cJSON *root = cJSON_CreateObject();

	// coveragePathPlan
	cJSON *coverage_path_plan_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "coveragePathPlan", coverage_path_plan_obj);
	cJSON *segments_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(coverage_path_plan_obj, "segments", segments_arr);

	int segment_id = 0;

	// Start transit segment (start_point → first headland or coverage waypoint)
	if (start_nav != NULL && cvector_size(start_nav) > 0)
	{
		cJSON *jsegment = cJSON_CreateObject();
		cJSON_AddNumberToObject(jsegment, "id", segment_id++);
		cJSON_AddStringToObject(jsegment, "type", headland != NULL ? "headlandTransit" : "coverageTransit");
		cJSON *path_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(jsegment, "path", path_arr);
		int pt_count = (int)cvector_size(start_nav);
		for (int j = 0; j < pt_count; ++j)
		{
			const point_t *pt = &start_nav[j];
			cJSON *jpath_point = cJSON_CreateObject();
			cJSON_AddNumberToObject(jpath_point, "id", j + 1);
			cJSON *jpoint = cJSON_CreateObject();
			cJSON_AddNumberToObject(jpoint, "x", pt->x);
			cJSON_AddNumberToObject(jpoint, "y", pt->y);
			cJSON_AddItemToObject(jpath_point, "point", jpoint);
			cJSON_AddItemToArray(path_arr, jpath_point);
		}
		cJSON_AddItemToArray(segments_arr, jsegment);
	}

	// Headland segments (prepended before coverage/transit)
	if (headland && headland->sections)
	{
		int section_count = (int)cvector_size(headland->sections);
		for (int i = 0; i < section_count; ++i)
		{
			const headland_section_t *hs = &headland->sections[i];
			if (hs->path == NULL || cvector_size(hs->path) == 0)
				continue;

			cJSON *jsegment = cJSON_CreateObject();
			cJSON_AddNumberToObject(jsegment, "id", segment_id++);
			cJSON_AddStringToObject(jsegment, "type", "headland");
			cJSON *path_arr = cJSON_CreateArray();
			cJSON_AddItemToObject(jsegment, "path", path_arr);
			int pt_count = (int)cvector_size(hs->path);
			for (int j = 0; j < pt_count; ++j)
			{
				const point_t *pt = &hs->path[j];
				cJSON *jpath_point = cJSON_CreateObject();
				cJSON_AddNumberToObject(jpath_point, "id", j + 1);
				cJSON *jpoint = cJSON_CreateObject();
				cJSON_AddNumberToObject(jpoint, "x", pt->x);
				cJSON_AddNumberToObject(jpoint, "y", pt->y);
				cJSON_AddItemToObject(jpath_point, "point", jpoint);
				cJSON_AddItemToArray(path_arr, jpath_point);
			}
			cJSON_AddItemToArray(segments_arr, jsegment);

			// headland transit segment (connects this section to next, or to first coverage point)
			if (hs->nav != NULL && cvector_size(hs->nav) > 0)
			{
				cJSON *jtransit = cJSON_CreateObject();
				cJSON_AddNumberToObject(jtransit, "id", segment_id++);
				cJSON_AddStringToObject(jtransit, "type", "headlandTransit");
				cJSON *nav_arr = cJSON_CreateArray();
				cJSON_AddItemToObject(jtransit, "path", nav_arr);
				int nav_count = (int)cvector_size(hs->nav);
				for (int j = 0; j < nav_count; ++j)
				{
					const point_t *pt = &hs->nav[j];
					cJSON *jnav_point = cJSON_CreateObject();
					cJSON_AddNumberToObject(jnav_point, "id", j + 1);
					cJSON *jpoint = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpoint, "x", pt->x);
					cJSON_AddNumberToObject(jpoint, "y", pt->y);
					cJSON_AddItemToObject(jnav_point, "point", jpoint);
					cJSON_AddItemToArray(nav_arr, jnav_point);
				}
				cJSON_AddItemToArray(segments_arr, jtransit);
			}
		}
	}
	{
		int section_count = cvector_size(motion_plan->section);
		for (int i = 0; i < section_count; ++i)
		{
			const cell_motion_plan_t *section = &motion_plan->section[i];

			// coverage segment
			if (section->ox && cvector_size(section->ox) > 0)
			{
				cJSON *jsegment = cJSON_CreateObject();
				cJSON_AddNumberToObject(jsegment, "id", segment_id++);
				cJSON_AddStringToObject(jsegment, "type", "coverage");
				cJSON *path_arr = cJSON_CreateArray();
				cJSON_AddItemToObject(jsegment, "path", path_arr);
				int point_count = cvector_size(section->ox);
				for (int j = 0; j < point_count; ++j)
				{
					const point_t *point = &section->ox[j];
					cJSON *jpath_point = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpath_point, "id", j + 1);
					cJSON *jpoint = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpoint, "x", point->x);
					cJSON_AddNumberToObject(jpoint, "y", point->y);
					cJSON_AddItemToObject(jpath_point, "point", jpoint);
					cJSON_AddItemToArray(path_arr, jpath_point);
				}
				cJSON_AddItemToArray(segments_arr, jsegment);
			}

			// coverage transit segment
			if (section->nav && cvector_size(section->nav) > 0)
			{
				cJSON *jsegment = cJSON_CreateObject();
				cJSON_AddNumberToObject(jsegment, "id", segment_id++);
				cJSON_AddStringToObject(jsegment, "type", "coverageTransit");
				cJSON *path_arr = cJSON_CreateArray();
				cJSON_AddItemToObject(jsegment, "path", path_arr);
				int nav_count = cvector_size(section->nav);
				for (int j = 0; j < nav_count; ++j)
				{
					const point_t *point = &section->nav[j];
					cJSON *jpath_point = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpath_point, "id", j + 1);
					cJSON *jpoint = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpoint, "x", point->x);
					cJSON_AddNumberToObject(jpoint, "y", point->y);
					cJSON_AddItemToObject(jpath_point, "point", jpoint);
					cJSON_AddItemToArray(path_arr, jpath_point);
				}
				cJSON_AddItemToArray(segments_arr, jsegment);
			}
		}
	}

	// debug
	cJSON *debug_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "debug", debug_obj);

	cJSON *layers_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(debug_obj, "layers", layers_arr);

	/* ---- Event List (id=10, source="eventList") ---- */
	{
		cJSON *event_layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(event_layer, "id", 10);
		cJSON_AddStringToObject(event_layer, "source", "eventList");
		cJSON *event_data_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(event_layer, "list", event_data_arr);

		if (event_list && event_list->bcd_events && event_list->length > 0)
		{
			for (int i = 0; i < event_list->length; ++i)
			{
				const bcd_event_t *ev = &event_list->bcd_events[i];
				cJSON *jev = cJSON_CreateObject();
				cJSON_AddNumberToObject(jev, "id", i + 1);
				cJSON *jv = cJSON_CreateObject();
				cJSON_AddNumberToObject(jv, "x", ev->polygon_vertex.x);
				cJSON_AddNumberToObject(jv, "y", ev->polygon_vertex.y);
				cJSON_AddItemToObject(jev, "point", jv);
				cJSON_AddStringToObject(jev, "pointLabel", event_type_to_string(ev->bcd_event_type));

				cJSON_AddItemToArray(event_data_arr, jev);
			}
		}

		cJSON_AddItemToArray(layers_arr, event_layer);
	}

	/* ---- Cell List (id=11, source="cellList") ---- */
	{
		cJSON *cell_layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(cell_layer, "id", 11);
		cJSON_AddStringToObject(cell_layer, "source", "cellList");
		cJSON *cell_data_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(cell_layer, "list", cell_data_arr);

		if (cell_list && *cell_list)
		{
			int cell_count = cvector_size(*cell_list);
			for (int i = 0; i < cell_count; ++i)
			{
				const bcd_cell_t *cell = &(*cell_list)[i];
				cJSON *jcell = cJSON_CreateObject();

				cJSON_AddNumberToObject(jcell, "id", i + 1);

				/*
				 * Build the full CW polygon boundary.
				 *
				 * Ceiling (top boundary, left → right):
				 *   c_begin (top-left), edge[j].end for j=0..n-2 (CEILING event
				 *   deflection vertices), c_end (top-right).
				 *
				 * Floor (bottom boundary, right → left, staying CW):
				 *   f_begin (bottom-right; shares the right side with c_end),
				 *   edge[j].end for j=0..n-2 (FLOOR event deflection vertices),
				 *   f_end (bottom-left; shares the left side with c_begin).
				 *
				 * Simple trapezoids (1 edge per list) produce exactly 4 points.
				 * SIDE_IN/SIDE_OUT tip cells have c_begin==f_end and c_end==f_begin,
				 * so consecutive-duplicate removal collapses them to 3 vertices.
				 *
				 * Capacity: 2 + (ceil_n-1) + 2 + (floor_n-1) = ceil_n + floor_n + 2
				 */
				int ceil_n = (int)cvector_size(cell->ceiling_edge_list);
				int floor_n = (int)cvector_size(cell->floor_edge_list);
				int raw_cap = ceil_n + floor_n + 2;

				cvector_vector_type(point_t) raw_pts = NULL;
				cvector_reserve(raw_pts, (size_t)raw_cap);

				/* ceiling: c_begin (top-left) */
				cvector_push_back(raw_pts, cell->c_begin);
				/* ceiling: intermediate deflection vertices (left → right) */
				for (int j = 0; j < ceil_n - 1; ++j)
					cvector_push_back(raw_pts, cell->ceiling_edge_list[j].end);
				/* ceiling: c_end (top-right) */
				cvector_push_back(raw_pts, cell->c_end);
				/* floor: f_begin (bottom-right; c_end and f_begin share the right side) */
				cvector_push_back(raw_pts, cell->f_begin);
				/* floor: intermediate deflection vertices (right → left, CW).
				 * Floor chain: edge[j].begin = edge[j+1].end, so reading right→left
				 * the intermediates are edge[j].end for j = floor_n-1 down to 1.
				 * (edge[0].end = f_end, already emitted separately below.) */
				for (int j = floor_n - 1; j >= 1; --j)
					cvector_push_back(raw_pts, cell->floor_edge_list[j].end);
				/* floor: f_end (bottom-left; f_end and c_begin share the left side) */
				cvector_push_back(raw_pts, cell->f_end);

				/* Remove consecutive duplicate points (handles triangle/degenerate cells).
				 * Also check wrap-around: if last == first, drop the last point. */
				cvector_vector_type(point_t) pts = NULL;
				int raw_cnt = (int)cvector_size(raw_pts);
				for (int j = 0; j < raw_cnt; ++j)
				{
					int prev = (j == 0) ? raw_cnt - 1 : j - 1;
					if (raw_pts[j].x != raw_pts[prev].x || raw_pts[j].y != raw_pts[prev].y)
						cvector_push_back(pts, raw_pts[j]);
				}
				cvector_free(raw_pts);

				cJSON *vertices = cJSON_CreateArray();
				int pt_count = (int)cvector_size(pts);
				for (int j = 0; j < pt_count; ++j)
				{
					cJSON *jpt = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpt, "x", pts[j].x);
					cJSON_AddNumberToObject(jpt, "y", pts[j].y);
					cJSON_AddItemToArray(vertices, jpt);
				}
				cvector_free(pts);
				cJSON_AddItemToObject(jcell, "vertices", vertices);

				point_t cp = bcd_cell_farthest_interior_point(cell);
				cJSON *jpoint = cJSON_CreateObject();
				cJSON_AddNumberToObject(jpoint, "x", cp.x);
				cJSON_AddNumberToObject(jpoint, "y", cp.y);
				cJSON_AddItemToObject(jcell, "centroidPoint", jpoint);

				cJSON_AddItemToArray(cell_data_arr, jcell);
			}
		}

		cJSON_AddItemToArray(layers_arr, cell_layer);
	}

	/* ---- Cell Visit Order (id=12, source="cellVisitOrder") ---- */
	/* Emits one CellVisitEntry per visited cell. The renderer connects them in order to form a line. */
	{
		cJSON *visit_layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(visit_layer, "id", 12);
		cJSON_AddStringToObject(visit_layer, "source", "cellVisitOrder");
		cJSON *visit_data_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(visit_layer, "list", visit_data_arr);

		if (path_list && *path_list)
		{
			int path_count = cvector_size(*path_list);
			for (int i = 0; i < path_count; ++i)
			{
				int cell_id = (*path_list)[i];
				point_t p = bcd_cell_farthest_interior_point(&(*cell_list)[cell_id]);

				cJSON *entry = cJSON_CreateObject();
				cJSON_AddNumberToObject(entry, "id", i + 1);
				cJSON_AddNumberToObject(entry, "pointLabel", cell_id + 1);

				cJSON *jpoint = cJSON_CreateObject();
				cJSON_AddNumberToObject(jpoint, "x", p.x);
				cJSON_AddNumberToObject(jpoint, "y", p.y);
				cJSON_AddItemToObject(entry, "point", jpoint);

				cJSON_AddItemToArray(visit_data_arr, entry);
			}
		}

		cJSON_AddItemToArray(layers_arr, visit_layer);
	}

	/* ---- performance is attached by bcd_run_compute after tracking
	 * is disabled and bcd_result_free has released all compute data. ---- */

	return root;
}

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

	va_tracking_mark("Paruošimas");
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

	va_tracking_mark("Įvykiai");
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
	va_tracking_mark("Ląstelės");
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
	va_tracking_mark("Ląstelių seka");
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

	va_tracking_mark("Maršrutas");
	// --- Build one visibility graph for ALL transit segments -----------
	//
	// All coverage section start/end points, headland section start/end points,
	// and the global start/end points are now known.  Inject every one of them
	// as extra nodes so the single O(n²) edge-validity sweep covers them all.
	// Subsequent A* queries just read the pre-computed adjacency matrix.
	//
	// Transit segments that need paths:
	//   (a) coverage section[i] end → coverage section[i+1] start
	//   (b) coverage section[last] end → env->end_point  (only when env->headland == false)
	//   (c) headland section[last] end → coverage section[0] start  (unused: headland pass is disabled)
	//   (d) env->start_point → coverage section[0] start

	cvector_vector_type(point_t) all_transit_nodes = NULL;

	// Always include global start.
	cvector_push_back(all_transit_nodes, env->start_point);
	// Include end_point only when headland is off — when headland is on, coverage
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
			// else: headland=ON — stop at last coverage point, nav stays NULL.

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

	// --- (c) Headland section[last] end → coverage section[0] start ---
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

	// --- (d) Start point → first headland or coverage waypoint ---
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

	/* Package results for the orchestrator — serialization and cleanup happen
	 * after tracking is disabled in bcd_run_compute (mirrors the C* pattern). */
	bcd_result_t *result = bcd_result_create();
	if (result == NULL)
	{
		vg_graph_free(vg);
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
	result->event_list = event_list;
	result->cell_list = cell_list;
	result->path_list = path_list;
	result->motion_plan = motion_plan;
	result->has_headland = has_headland;
	if (has_headland)
		result->headland = headland;
	result->start_nav = start_nav;
	result->vg = vg;
	return result;
}

static const char *event_type_to_string(bcd_event_type_t t)
{
	switch (t)
	{
	case BCD_B_INIT:
		return "B_INIT";
	case BCD_B_DEINIT:
		return "B_DEINIT";
	case BCD_B_IN:
		return "B_IN";
	case BCD_B_OUT:
		return "B_OUT";
	case BCD_B_SIDE_IN:
		return "B_SIDE_IN";
	case BCD_B_SIDE_OUT:
		return "B_SIDE_OUT";
	case BCD_IN:
		return "IN";
	case BCD_SIDE_IN:
		return "SIDE_IN";
	case BCD_OUT:
		return "OUT";
	case BCD_SIDE_OUT:
		return "SIDE_OUT";
	case BCD_FLOOR:
		return "FLOOR";
	case BCD_CEILING:
		return "CEILING";
	default:
		return "UNKNOWN";
	}
}

static const char *polygon_type_to_string(polygon_type_t t)
{
	switch (t)
	{
	case BOUNDARY:
		return "BOUNDARY";
	case OBSTACLE:
		return "OBSTACLE";
	default:
		return "UNKNOWN";
	}
}

static char *serialize_event_list_json(const bcd_event_list_t *event_list)
{
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "status", "ok");
	cJSON *arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "event_list", arr);

	if (event_list && event_list->bcd_events && event_list->length > 0)
	{
		for (int i = 0; i < event_list->length; ++i)
		{
			const bcd_event_t *ev = &event_list->bcd_events[i];
			cJSON *jev = cJSON_CreateObject();
			cJSON_AddStringToObject(jev, "polygon_type", polygon_type_to_string(ev->polygon_type));
			cJSON *jv = cJSON_CreateObject();
			cJSON_AddNumberToObject(jv, "x", ev->polygon_vertex.x);
			cJSON_AddNumberToObject(jv, "y", ev->polygon_vertex.y);
			cJSON_AddItemToObject(jev, "vertex", jv);
			cJSON_AddStringToObject(jev, "event_type", event_type_to_string(ev->bcd_event_type));

			// floor edge
			cJSON *jfloor = cJSON_CreateObject();
			cJSON *jfb = cJSON_CreateObject();
			cJSON_AddNumberToObject(jfb, "x", ev->floor_edge.begin.x);
			cJSON_AddNumberToObject(jfb, "y", ev->floor_edge.begin.y);
			cJSON_AddItemToObject(jfloor, "begin", jfb);
			cJSON *jfe = cJSON_CreateObject();
			cJSON_AddNumberToObject(jfe, "x", ev->floor_edge.end.x);
			cJSON_AddNumberToObject(jfe, "y", ev->floor_edge.end.y);
			cJSON_AddItemToObject(jfloor, "end", jfe);
			cJSON_AddItemToObject(jev, "floor_edge", jfloor);

			// ceiling edge
			cJSON *jceil = cJSON_CreateObject();
			cJSON *jcb = cJSON_CreateObject();
			cJSON_AddNumberToObject(jcb, "x", ev->ceiling_edge.begin.x);
			cJSON_AddNumberToObject(jcb, "y", ev->ceiling_edge.begin.y);
			cJSON_AddItemToObject(jceil, "begin", jcb);
			cJSON *jce = cJSON_CreateObject();
			cJSON_AddNumberToObject(jce, "x", ev->ceiling_edge.end.x);
			cJSON_AddNumberToObject(jce, "y", ev->ceiling_edge.end.y);
			cJSON_AddItemToObject(jceil, "end", jce);
			cJSON_AddItemToObject(jev, "ceiling_edge", jceil);

			cJSON_AddItemToArray(arr, jev);
		}
	}

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json; // caller must free
}

static void log_event_list(const bcd_event_list_t *event_list)
{
	// printf("coverage_path_planning: successfully generated %d events\n", event_list->length);

	if (event_list->bcd_events != NULL && event_list->length > 0)
	{
		LOG_DEBUG("coverage_path_planning: event list preview:");
		for (int i = 0; i < event_list->length; i++)
		{
			const char *type_str = "UNKNOWN";
			switch (event_list->bcd_events[i].bcd_event_type)
			{
			case BCD_B_INIT:
				type_str = "B_INIT";
				break;
			case BCD_B_DEINIT:
				type_str = "B_DEINIT";
				break;
			case BCD_B_IN:
				type_str = "B_IN";
				break;
			case BCD_B_OUT:
				type_str = "B_OUT";
				break;
			case BCD_B_SIDE_IN:
				type_str = "B_SIDE_IN";
				break;
			case BCD_B_SIDE_OUT:
				type_str = "B_SIDE_OUT";
				break;
			case BCD_IN:
				type_str = "IN";
				break;
			case BCD_SIDE_IN:
				type_str = "SIDE_IN";
				break;
			case BCD_OUT:
				type_str = "OUT";
				break;
			case BCD_SIDE_OUT:
				type_str = "SIDE_OUT";
				break;
			case BCD_FLOOR:
				type_str = "FLOOR";
				break;
			case BCD_CEILING:
				type_str = "CEILING";
				break;
			case BCD_NONE:
				break;
			}
			LOG_DEBUG("  Event %d: (%.2f, %.2f) type=%s polygon=%s",
					  i, event_list->bcd_events[i].polygon_vertex.x, event_list->bcd_events[i].polygon_vertex.y,
					  type_str, event_list->bcd_events[i].polygon_type == BOUNDARY ? "BOUNDARY" : "OBSTACLE");
		}
	}
}

// POINT_T helpers

bool are_equal_points(point_t a,
					  point_t b)
{
	const float eps = 1e-4f;
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps;
}
