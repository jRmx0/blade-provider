#include <stdio.h>
#include <stdbool.h>
#include "bcd_runner.h"
#include "../../../dependencies/cJSON/cJSON.h"
#include "../../../dependencies/cvector/cvector.h"
#include "bcd_core/bcd_event_list_building.h"
#include "bcd_core/bcd_cell_computation.h"
#include "bcd_core/bcd_coverage_planning.h"
#include "bcd_core/bcd_motion_planning.h"

static void log_event_list(const bcd_event_list_t *event_list);
static const char *event_type_to_string(bcd_event_type_t t);
static const char *polygon_type_to_string(polygon_type_t t);
static char *serialize_event_list_json(const bcd_event_list_t *event_list);
static char *serialize_result_json(const bcd_event_list_t *event_list,
								   cvector_vector_type(bcd_cell_t) * cell_list,
								   cvector_vector_type(int) * path_list,
								   const bcd_motion_plan_t *motion_plan);
static char *err_cleanup(bcd_event_list_t *event_list,
						 cvector_vector_type(bcd_cell_t) * cell_list,
						 cvector_vector_type(int) * path_list,
						 bcd_motion_plan_t *motion_plan,
						 int rc);

char *coverage_path_planning_process(const input_environment_t *env)
{
	bcd_event_list_t event_list;
	event_list.bcd_events = NULL;
	event_list.length = 0;

	int rc = build_bcd_event_list(env, &event_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD event list generation failed (code %d)\n", rc);
		return err_cleanup(&event_list, NULL, NULL, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated %d events\n", event_list.length);

	cvector_vector_type(bcd_cell_t) cell_list = NULL;
	rc = compute_bcd_cells(&event_list, &cell_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD cell computation failed (code %d)\n", rc);
		return err_cleanup(&event_list, &cell_list, NULL, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated %zu cells\n", cvector_size(cell_list));
	// log_bcd_cell_list((const cvector_vector_type(bcd_cell_t) *) &cell_list);

	cvector_vector_type(int) path_list = NULL;
	rc = compute_bcd_path_list(&cell_list, -1, &path_list);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD path computation failed (code %d)\n", rc);
		return err_cleanup(&event_list, &cell_list, &path_list, NULL, rc);
	}
	printf("coverage_path_planning: successfully generated path with %zu visits\n", cvector_size(path_list));
	// log_bcd_path_list((const cvector_vector_type(int) *)&path_list);

	bcd_motion_plan_t motion_plan = {0};
	rc = compute_bcd_motion(&cell_list,
							(const cvector_vector_type(int) *)&path_list,
							&motion_plan,
							0.25);
	if (rc != 0)
	{
		printf("coverage_path_planning: BCD motion computation failed (code %d)\n", rc);
		return err_cleanup(&event_list, &cell_list, &path_list, &motion_plan, rc);
	}
	log_bcd_motion(motion_plan);

	char *json_out = serialize_result_json(&event_list, &cell_list, &path_list, &motion_plan);

	err_cleanup(&event_list, &cell_list, &path_list, &motion_plan, rc);

	return json_out;
}



static const char *event_type_to_string(bcd_event_type_t t)
{
	switch (t)
	{
	case B_INIT:
		return "B_INIT";
	case B_DEINIT:
		return "B_DEINIT";
	case B_IN:
		return "B_IN";
	case B_OUT:
		return "B_OUT";
	case B_SIDE_IN:
		return "B_SIDE_IN";
	case B_SIDE_OUT:
		return "B_SIDE_OUT";
	case IN:
		return "IN";
	case SIDE_IN:
		return "SIDE_IN";
	case OUT:
		return "OUT";
	case SIDE_OUT:
		return "SIDE_OUT";
	case FLOOR:
		return "FLOOR";
	case CEILING:
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

static char *serialize_result_json(const bcd_event_list_t *event_list,
								   cvector_vector_type(bcd_cell_t) * cell_list,
								   cvector_vector_type(int) * path_list,
								   const bcd_motion_plan_t *motion_plan)
{
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "status", "ok");

	// Add event list
	cJSON *event_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "event_list", event_arr);

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

			cJSON_AddItemToArray(event_arr, jev);
		}
	}

	// Add cell list
	cJSON *cell_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "cell_list", cell_arr);

	if (cell_list && *cell_list)
	{
		int cell_count = cvector_size(*cell_list);
		for (int i = 0; i < cell_count; ++i)
		{
			const bcd_cell_t *cell = &(*cell_list)[i];
			cJSON *jcell = cJSON_CreateObject();

			// Add cell number
			cJSON_AddNumberToObject(jcell, "cell_number", i);

			// Add ceiling boundary
			cJSON *jc_begin = cJSON_CreateObject();
			cJSON_AddNumberToObject(jc_begin, "x", cell->c_begin.x);
			cJSON_AddNumberToObject(jc_begin, "y", cell->c_begin.y);
			cJSON_AddItemToObject(jcell, "c_begin", jc_begin);

			cJSON *jc_end = cJSON_CreateObject();
			cJSON_AddNumberToObject(jc_end, "x", cell->c_end.x);
			cJSON_AddNumberToObject(jc_end, "y", cell->c_end.y);
			cJSON_AddItemToObject(jcell, "c_end", jc_end);

			// Add floor boundary
			cJSON *jf_begin = cJSON_CreateObject();
			cJSON_AddNumberToObject(jf_begin, "x", cell->f_begin.x);
			cJSON_AddNumberToObject(jf_begin, "y", cell->f_begin.y);
			cJSON_AddItemToObject(jcell, "f_begin", jf_begin);

			cJSON *jf_end = cJSON_CreateObject();
			cJSON_AddNumberToObject(jf_end, "x", cell->f_end.x);
			cJSON_AddNumberToObject(jf_end, "y", cell->f_end.y);
			cJSON_AddItemToObject(jcell, "f_end", jf_end);

			// Add ceiling edges
			cJSON *ceiling_edges = cJSON_CreateArray();
			if (cell->ceiling_edge_list)
			{
				int ceiling_edge_count = cvector_size(cell->ceiling_edge_list);
				for (int j = 0; j < ceiling_edge_count; ++j)
				{
					const polygon_edge_t *edge = &cell->ceiling_edge_list[j];
					cJSON *jedge = cJSON_CreateObject();
					cJSON *jbegin = cJSON_CreateObject();
					cJSON_AddNumberToObject(jbegin, "x", edge->begin.x);
					cJSON_AddNumberToObject(jbegin, "y", edge->begin.y);
					cJSON_AddItemToObject(jedge, "begin", jbegin);
					cJSON *jend = cJSON_CreateObject();
					cJSON_AddNumberToObject(jend, "x", edge->end.x);
					cJSON_AddNumberToObject(jend, "y", edge->end.y);
					cJSON_AddItemToObject(jedge, "end", jend);
					cJSON_AddItemToArray(ceiling_edges, jedge);
				}
			}
			cJSON_AddItemToObject(jcell, "ceiling_edges", ceiling_edges);

			// Add floor edges
			cJSON *floor_edges = cJSON_CreateArray();
			if (cell->floor_edge_list)
			{
				int floor_edge_count = cvector_size(cell->floor_edge_list);
				for (int j = 0; j < floor_edge_count; ++j)
				{
					const polygon_edge_t *edge = &cell->floor_edge_list[j];
					cJSON *jedge = cJSON_CreateObject();
					cJSON *jbegin = cJSON_CreateObject();
					cJSON_AddNumberToObject(jbegin, "x", edge->begin.x);
					cJSON_AddNumberToObject(jbegin, "y", edge->begin.y);
					cJSON_AddItemToObject(jedge, "begin", jbegin);
					cJSON *jend = cJSON_CreateObject();
					cJSON_AddNumberToObject(jend, "x", edge->end.x);
					cJSON_AddNumberToObject(jend, "y", edge->end.y);
					cJSON_AddItemToObject(jedge, "end", jend);
					cJSON_AddItemToArray(floor_edges, jedge);
				}
			}
			cJSON_AddItemToObject(jcell, "floor_edges", floor_edges);

			// Add cell properties
			cJSON_AddBoolToObject(jcell, "open", cell->open);
			cJSON_AddBoolToObject(jcell, "visited", cell->visited);
			cJSON_AddBoolToObject(jcell, "cleaned", cell->cleaned);

			cJSON_AddItemToArray(cell_arr, jcell);
		}
	}

	// Add path list
	cJSON *path_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "path_list", path_arr);

	if (path_list && *path_list)
	{
		int path_count = cvector_size(*path_list);
		for (int i = 0; i < path_count; ++i)
		{
			cJSON_AddItemToArray(path_arr, cJSON_CreateNumber((*path_list)[i]));
		}
	}

	// Add motion plan
	cJSON *motion_plan_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "motion_plan", motion_plan_obj);

	cJSON *sections_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(motion_plan_obj, "sections", sections_arr);

	if (motion_plan && motion_plan->section)
	{
		int section_count = cvector_size(motion_plan->section);
		for (int i = 0; i < section_count; ++i)
		{
			const cell_motion_plan_t *section = &motion_plan->section[i];
			cJSON *jsection = cJSON_CreateObject();
			cJSON_AddNumberToObject(jsection, "section_id", i);

			// Add coverage points (ox)
			cJSON *coverage_arr = cJSON_CreateArray();
			cJSON_AddItemToObject(jsection, "coverage", coverage_arr);

			if (section->ox)
			{
				int point_count = cvector_size(section->ox);
				for (int j = 0; j < point_count; ++j)
				{
					const point_t *point = &section->ox[j];
					cJSON *jpoint = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpoint, "x", point->x);
					cJSON_AddNumberToObject(jpoint, "y", point->y);
					cJSON_AddItemToArray(coverage_arr, jpoint);
				}
			}

			// Add navigation points (nav)
			cJSON *nav_arr = cJSON_CreateArray();
			cJSON_AddItemToObject(jsection, "navigation", nav_arr);

			if (section->nav)
			{
				int nav_count = cvector_size(section->nav);
				for (int j = 0; j < nav_count; ++j)
				{
					const point_t *point = &section->nav[j];
					cJSON *jpoint = cJSON_CreateObject();
					cJSON_AddNumberToObject(jpoint, "x", point->x);
					cJSON_AddNumberToObject(jpoint, "y", point->y);
					cJSON_AddItemToArray(nav_arr, jpoint);
				}
			}

			cJSON_AddItemToArray(sections_arr, jsection);
		}
	}

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json; // caller must free
}

static char *err_cleanup(bcd_event_list_t *event_list,
						 cvector_vector_type(bcd_cell_t) * cell_list,
						 cvector_vector_type(int) * path_list,
						 bcd_motion_plan_t *motion_plan,
						 int rc)
{
	free_bcd_event_list(event_list);
	free_bcd_cell_list(cell_list);
	if (path_list) cvector_free(*path_list);
	free_bcd_motion(motion_plan);

	cJSON *err = cJSON_CreateObject();
	cJSON_AddStringToObject(err, "status", "error");
	cJSON_AddNumberToObject(err, "code", rc);
	cJSON_AddStringToObject(err, "message", "BCD computation failed");
	char *out = cJSON_PrintUnformatted(err);
	cJSON_Delete(err);
	return out;
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
			case B_INIT:
				type_str = "B_INIT";
				break;
			case B_DEINIT:
				type_str = "B_DEINIT";
				break;
			case B_IN:
				type_str = "B_IN";
				break;
			case B_OUT:
				type_str = "B_OUT";
				break;
			case B_SIDE_IN:
				type_str = "B_SIDE_IN";
				break;
			case B_SIDE_OUT:
				type_str = "B_SIDE_OUT";
				break;
			case IN:
				type_str = "IN";
				break;
			case SIDE_IN:
				type_str = "SIDE_IN";
				break;
			case OUT:
				type_str = "OUT";
				break;
			case SIDE_OUT:
				type_str = "SIDE_OUT";
				break;
			case FLOOR:
				type_str = "FLOOR";
				break;
			case CEILING:
				type_str = "CEILING";
				break;
			case NONE:
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
	return a.x == b.x && a.y == b.y;
}


