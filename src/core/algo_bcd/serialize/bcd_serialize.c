/**
 * bcd_serialize.c
 *
 * Serializes a bcd_result_t into a cJSON tree.
 * Included by compute/runner/bcd_runner.c after all algorithm types are defined.
 *
 * Dependencies: all types and headers pulled in transitively by bcd_runner.c
 */

#include "bcd_serialize.h"
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
