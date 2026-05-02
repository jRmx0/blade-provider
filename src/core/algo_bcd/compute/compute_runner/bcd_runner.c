#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "bcd_runner.h"
#include "../../../../../dependencies/cJSON/cJSON.h"
#include "../../../../../dependencies/cvector/cvector.h"
#include "bcd_core/bcd_event_list_building.h"
#include "bcd_core/bcd_cell_computation.h"
#include "bcd_core/bcd_coverage_planning.h"
#include "bcd_core/bcd_motion_planning.h"
#include "bcd_core/bcd_geometry.h"
#include "../../../common/headland.h"
#include "../../../common/path_finder.h"
#include "../../preprocess/bcd_preprocess.h"

#include "../../../common/polygon_offset.c"
#include "../../../common/path_finder.c"
#include "../../../common/headland.c"

static void log_event_list(const bcd_event_list_t *event_list);
static const char *event_type_to_string(bcd_event_type_t t);
static const char *polygon_type_to_string(polygon_type_t t);
static char *serialize_event_list_json(const bcd_event_list_t *event_list);

static cJSON *serialize_result_json(const bcd_event_list_t *event_list,
									cvector_vector_type(bcd_cell_t) * cell_list,
									cvector_vector_type(int) * path_list,
									const bcd_motion_plan_t *motion_plan,
									const headland_t *headland,
									cvector_vector_type(point_t) start_nav)
{
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

	/* ---- Shrunken Zone Border (id=13, source="headlandShrunkenZoneBorder") ---- */
	{
		cJSON *zone_layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(zone_layer, "id", 13);
		cJSON_AddStringToObject(zone_layer, "source", "headlandShrunkenZoneBorder");
		cJSON *zone_data_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(zone_layer, "list", zone_data_arr);

		if (headland && headland->shrunken_zone.vertices && headland->shrunken_zone.vertex_count > 0)
		{
			cJSON *entry = cJSON_CreateObject();
			cJSON_AddNumberToObject(entry, "id", 1);

			cJSON *vertices = cJSON_CreateArray();
			for (uint32_t i = 0; i < headland->shrunken_zone.vertex_count; ++i)
			{
				cJSON *jpt = cJSON_CreateObject();
				cJSON_AddNumberToObject(jpt, "x", headland->shrunken_zone.vertices[i].x);
				cJSON_AddNumberToObject(jpt, "y", headland->shrunken_zone.vertices[i].y);
				cJSON_AddItemToArray(vertices, jpt);
			}
			cJSON_AddItemToObject(entry, "vertices", vertices);

			cJSON_AddItemToArray(zone_data_arr, entry);
		}

		cJSON_AddItemToArray(layers_arr, zone_layer);
	}

	/* ---- Expanded Obstacle Borders (id=14, source="headlandExpandedObstacleBorders") ---- */
	{
		cJSON *obs_layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(obs_layer, "id", 14);
		cJSON_AddStringToObject(obs_layer, "source", "headlandExpandedObstacleBorders");
		cJSON *obs_data_arr = cJSON_CreateArray();
		cJSON_AddItemToObject(obs_layer, "list", obs_data_arr);

		if (headland && headland->expanded_obstacles && headland->expanded_obstacle_count > 0)
		{
			for (uint32_t k = 0; k < headland->expanded_obstacle_count; ++k)
			{
				const polygon_t *obs = &headland->expanded_obstacles[k];
				if (obs->vertices == NULL || obs->vertex_count == 0)
					continue;

				cJSON *entry = cJSON_CreateObject();
				cJSON_AddNumberToObject(entry, "id", (double)(k + 1));

				cJSON *vertices = cJSON_CreateArray();
				for (uint32_t i = 0; i < obs->vertex_count; ++i)
				{
					cJSON *jpt = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpt, "x", obs->vertices[i].x);
					cJSON_AddNumberToObject(jpt, "y", obs->vertices[i].y);
					cJSON_AddItemToArray(vertices, jpt);
				}
				cJSON_AddItemToObject(entry, "vertices", vertices);

				cJSON_AddItemToArray(obs_data_arr, entry);
			}
		}

		cJSON_AddItemToArray(layers_arr, obs_layer);
	}

	/* ---- performance is injected by bcd_run_compute after all
	 * compute data and environment polygons have been freed, so
	 * the working-set drop from those releases is captured first. ---- */

	return root;
}

static cJSON *err_cleanup(bcd_event_list_t *event_list,
						  cvector_vector_type(bcd_cell_t) * cell_list,
						  cvector_vector_type(int) * path_list,
						  bcd_motion_plan_t *motion_plan,
						  int rc);

cJSON *coverage_path_planning_process(input_environment_t *env)
{
	bcd_event_list_t event_list;
	event_list.bcd_events = NULL;
	event_list.length = 0;

	// --- Headland pass (optional) ---
	headland_t headland;
	memset(&headland, 0, sizeof(headland_t));
	bool has_headland = false;

	input_environment_t headland_env;
	memset(&headland_env, 0, sizeof(input_environment_t));

	input_environment_t *active_env = env;

	if (env->headland)
	{
		int hrc = compute_headland(env, &headland);
		if (hrc != 0)
		{
			printf("coverage_path_planning: headland generation failed (code %d)\n", hrc);
			free_headland(&headland);

			const char *err_code =
				(hrc == -10) ? "obstacles_too_close" : (hrc == -11) ? "obstacle_too_close_to_boundary"
												   : (hrc == -3)	? "no_headland_transit_path"
																	: "headland_failed";
			const char *err_msg =
				(hrc == -10)   ? "Two or more obstacles are too close to each other: their expanded headland boundaries overlap. Reduce Path Width, Headland Coverage Offset, or increase the distance between obstacles."
				: (hrc == -11) ? "An obstacle is too close to the zone boundary: its expanded headland boundary escapes the shrunken zone. Reduce Path Width, Headland Coverage Offset, or move the obstacle away from the boundary."
				: (hrc == -3)  ? "No collision-free path could be found between headland sections. The field geometry may be too complex or obstacles too close together."
							   : "Headland generation failed.";

			cJSON *err = cJSON_CreateObject();
			cJSON_AddStringToObject(err, "status", "error");
			cJSON_AddStringToObject(err, "code", err_code);
			cJSON_AddStringToObject(err, "message", err_msg);
			return err;
		}
		else
		{
			has_headland = true;

			// Build a reduced input_environment_t pointing at the headland geometry.
			// This is a shallow wrapper — the polygon data is owned by headland.
			headland_env.id = env->id;
			headland_env.path_width = env->path_width;
			headland_env.path_overlap = env->path_overlap;
			headland_env.track_memory_usage = env->track_memory_usage;
			headland_env.headland = false; // already processed
			headland_env.start_point = env->start_point;
			headland_env.end_point = env->end_point;
			headland_env.boundary = headland.shrunken_zone;
			headland_env.obstacles = headland.expanded_obstacles;
			headland_env.obstacle_count = headland.expanded_obstacle_count;

			active_env = &headland_env;

			printf("coverage_path_planning: headland generated %zu section(s)\n",
				   cvector_size(headland.sections));
		}
	}

	int rc = bcd_preprocess_environment(active_env, 0.0f);
	if (rc != 0)
	{
		printf("coverage_path_planning: environment preprocessing failed (code %d)\n", rc);
		if (has_headland)
			free_headland(&headland);
		return err_cleanup(&event_list, NULL, NULL, NULL, rc);
	}

	rc = build_bcd_event_list(active_env, &event_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD event list generation failed (code %d)\n", rc);
		if (has_headland)
			free_headland(&headland);
		return err_cleanup(&event_list, NULL, NULL, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated %d events\n", event_list.length);
	cvector_vector_type(bcd_cell_t) cell_list = NULL;
	rc = compute_bcd_cells(&event_list, &cell_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD cell computation failed (code %d)\n", rc);
		if (has_headland)
			free_headland(&headland);
		return err_cleanup(&event_list, &cell_list, NULL, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated %zu cells\n", cvector_size(cell_list));
	// log_bcd_cell_list((const cvector_vector_type(bcd_cell_t) *) &cell_list);

	cvector_vector_type(int) path_list = NULL;
	int starting_cell_index = bcd_find_starting_cell(
		(const cvector_vector_type(bcd_cell_t) *)&cell_list,
		active_env->start_point);
	rc = compute_bcd_path_list(&cell_list, starting_cell_index, &path_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD path computation failed (code %d)\n", rc);
		if (has_headland)
			free_headland(&headland);
		return err_cleanup(&event_list, &cell_list, &path_list, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated path with %zu visits\n", cvector_size(path_list));
	// log_bcd_path_list((const cvector_vector_type(int) *)&path_list);

	bcd_motion_plan_t motion_plan = {0};
	rc = compute_bcd_motion(&cell_list,
							(const cvector_vector_type(int) *)&path_list,
							&motion_plan,
							active_env->path_width - active_env->path_overlap,
							active_env->end_point);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD motion computation failed (code %d)\n", rc);
		if (has_headland)
			free_headland(&headland);
		return err_cleanup(&event_list, &cell_list, &path_list, &motion_plan, rc);
	}
	log_bcd_motion(motion_plan);

	// --- Last coverage point → end_point transit ---
	// compute_bcd_motion sets the last section's nav via compute_connection_motion
	// (cell-spine).  Replace it with a VG A* path so that end_point values that
	// lie outside the BCD cells are handled correctly and obstacle crossing is
	// explicitly avoided — consistent with how start_point transit is handled.
	if (motion_plan.section != NULL && cvector_size(motion_plan.section) > 0)
	{
		int last_idx = (int)cvector_size(motion_plan.section) - 1;
		cell_motion_plan_t *last_sec = &motion_plan.section[last_idx];
		if (last_sec->ox != NULL && cvector_size(last_sec->ox) > 0)
		{
			point_t ep_from = last_sec->ox[cvector_size(last_sec->ox) - 1];
			point_t ep_to = active_env->end_point;

			cvector_vector_type(point_t) existing_nav = last_sec->nav;
			cvector_vector_type(point_t) replacement_nav =
				find_free_space_path(ep_from, ep_to, env, env->path_width / 2.0f);
			if (replacement_nav != NULL)
			{
				cvector_free(existing_nav);
				last_sec->nav = replacement_nav;
			}
			else if (!has_headland && existing_nav != NULL && cvector_size(existing_nav) > 0)
			{
				// Headland disabled: keep the cell-spine transit produced by
				// compute_bcd_motion when VG A* replacement cannot be found.
				last_sec->nav = existing_nav;
			}
			else
			{
				if (has_headland)
					free_headland(&headland);
				free_bcd_event_list(&event_list);
				free_bcd_cell_list(&cell_list);
				cvector_free(path_list);
				free_bcd_motion(&motion_plan);
				cJSON *err = cJSON_CreateObject();
				cJSON_AddStringToObject(err, "status", "error");
				cJSON_AddStringToObject(err, "code", "no_end_transit_path");
				cJSON_AddStringToObject(err, "message", "No collision-free path could be found from the last coverage point to the end point.");
				return err;
			}
		}
	}

	// --- Headland → first coverage point transit ---
	// The last headland section's nav must point to the first coverage waypoint.
	// This can only be computed here because motion_plan is not available inside
	// compute_headland.
	//
	// from_pt lies on the headland boundary, not inside any BCD cell, so
	// compute_connection_motion is not appropriate here — it degrades to a
	// direct line when from_pt falls outside the first cell's x-range.
	//
	// find_free_space_path uses the same half_width offset free-space as
	// compute_headland used for inter-section transit.  shrunken_zone is at
	// (path_width - path_overlap) — the BCD area boundary — which is larger than
	// the headland ring, so half_width is used rather than shrunken_zone.
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

				// Route from the last headland waypoint to the first coverage waypoint.
				// Uses half_width offset free-space (same as compute_headland internally).
				last_hs->nav = find_free_space_path(from_pt, to_pt, env, env->path_width / 2.0f);
				if (last_hs->nav == NULL)
				{
					free_headland(&headland);
					return err_cleanup(&event_list, &cell_list, &path_list, &motion_plan, -3);
				}
			}
		}
	}

	// --- Start point → first path waypoint transit ---
	// Bridges the gap from start_point to the first headland (or coverage) waypoint.
	// Emitted as the very first segment in coveragePathPlan.segments.
	cvector_vector_type(point_t) start_nav = NULL;

	if (has_headland)
	{
		// Route from start_point to the first headland waypoint.
		// Uses half_width offset free-space (same as compute_headland internally).
		if (headland.sections != NULL && cvector_size(headland.sections) > 0)
		{
			headland_section_t *first_hs = &headland.sections[0];
			if (first_hs->path != NULL && cvector_size(first_hs->path) > 0)
			{
				point_t sp_from = env->start_point;
				point_t sp_to = first_hs->path[0];

				start_nav = find_free_space_path(sp_from, sp_to, env, env->path_width / 2.0f);
				if (start_nav == NULL)
				{
					free_headland(&headland);
					return err_cleanup(&event_list, &cell_list, &path_list, &motion_plan, -3);
				}
			}
		}
	}
	else
	{
		// No headland: direct 2-point line from start_point to the first coverage waypoint.
		if (motion_plan.section != NULL && cvector_size(motion_plan.section) > 0 &&
			motion_plan.section[0].ox != NULL && cvector_size(motion_plan.section[0].ox) > 0)
		{
			point_t sp_from = active_env->start_point;
			point_t sp_to = motion_plan.section[0].ox[0];
			cvector_push_back(start_nav, sp_from);
			cvector_push_back(start_nav, sp_to);
		}
	}

	cJSON *root = serialize_result_json(&event_list, &cell_list, &path_list, &motion_plan,
										has_headland ? &headland : NULL,
										start_nav);

	/* Free compute data after serializing — these va_free calls are tracked,
	 * so the working-set drop from releasing cell/path/motion/event data
	 * appears in the sample vector before the snapshot is taken. */
	free_bcd_event_list(&event_list);
	free_bcd_cell_list(&cell_list);
	cvector_free(path_list);
	free_bcd_motion(&motion_plan);
	cvector_free(start_nav);

	if (has_headland)
		free_headland(&headland);

	return root;
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

static cJSON *err_cleanup(bcd_event_list_t *event_list,
						  cvector_vector_type(bcd_cell_t) * cell_list,
						  cvector_vector_type(int) * path_list,
						  bcd_motion_plan_t *motion_plan,
						  int rc)
{
	// NOTE:
	// Some malformed/degenerate inputs can leave partially-built internal
	// structures (event vectors / cell neighbor links / motion paths) in an
	// inconsistent state. Any deep free in this error path can crash.
	//
	// This compute worker handles a single job and exits immediately after
	// returning a response, so we intentionally skip deallocation on failure to
	// guarantee a stable JSON error instead of process abort.
	(void)event_list;
	(void)cell_list;
	(void)path_list;
	(void)motion_plan;

	cJSON *err = cJSON_CreateObject();
	cJSON_AddStringToObject(err, "status", "error");
	cJSON_AddNumberToObject(err, "code", rc);
	cJSON_AddStringToObject(err, "message", "BCD computation failed");
	return err;
}

static void log_event_list(const bcd_event_list_t *event_list)
{
	// printf("coverage_path_planning: successfully generated %d events\n", event_list->length);

	if (event_list->bcd_events != NULL && event_list->length > 0)
	{
		printf("coverage_path_planning: event list preview:\n");
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
			printf("  Event %d: (%.2f, %.2f) type=%s polygon=%s\n",
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
