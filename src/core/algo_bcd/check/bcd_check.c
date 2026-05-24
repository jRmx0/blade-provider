#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bcd_check.h"
#include "../../../../dependencies/cJSON/cJSON.h"
#include "../../../../dependencies/allocator/allocator.h"

static void bcd_init_environment(input_environment_t *environment)
{
	environment->id = 0;
	environment->path_width = 0.0f;
	environment->path_overlap = 0.0f;
	environment->boundary.winding = POLYGON_WINDING_UNKNOWN;
	environment->boundary.vertices = NULL;
	environment->boundary.vertex_count = 0;
	environment->boundary.edges = NULL;
	environment->boundary.edge_count = 0;
	environment->obstacles = NULL;
	environment->obstacle_count = 0;
	environment->track_memory_usage = false;
	environment->track_processing_time = false;
	environment->headland = false;
	environment->max_iterations = 0;
	environment->start_point.x = 0.0f;
	environment->start_point.y = 0.0f;
	environment->end_point.x = 0.0f;
	environment->end_point.y = 0.0f;
	environment->realworld_boundary.winding = POLYGON_WINDING_UNKNOWN;
	environment->realworld_boundary.vertices = NULL;
	environment->realworld_boundary.vertex_count = 0;
	environment->realworld_boundary.edges = NULL;
	environment->realworld_boundary.edge_count = 0;
	environment->realworld_obstacles = NULL;
	environment->realworld_obstacle_count = 0;
}

static void bcd_set_result(bcd_check_result_t *result, bool ok, const char *code, const char *message)
{
	if (result == NULL)
	{
		return;
	}

	result->ok = ok;
	result->code = code;
	result->message = message;
}

static bool bcd_parse_float_value(const cJSON *value, float *out)
{
	if (cJSON_IsNumber(value))
	{
		*out = (float)value->valuedouble;
		return true;
	}

	if (!cJSON_IsString(value) || value->valuestring == NULL)
	{
		return false;
	}

	const char *cursor = value->valuestring;
	while (*cursor != '\0' && isspace((unsigned char)*cursor))
	{
		cursor++;
	}

	if (*cursor == '\0')
	{
		return false;
	}

	char *end = NULL;
	float parsed = strtof(cursor, &end);
	if (cursor == end)
	{
		return false;
	}

	while (end != NULL && *end != '\0' && isspace((unsigned char)*end))
	{
		end++;
	}

	if (end == NULL || *end != '\0')
	{
		return false;
	}

	*out = parsed;
	return true;
}

static bool bcd_expect_string_parameter(
	const cJSON *parameters,
	const char *name,
	const char *expected_value,
	bcd_check_result_t *result,
	const char *missing_code,
	const char *missing_message,
	const char *invalid_code,
	const char *invalid_message)
{
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, name);
	if (!cJSON_IsString(value) || value->valuestring == NULL || value->valuestring[0] == '\0')
	{
		bcd_set_result(result, false, missing_code, missing_message);
		return false;
	}

	if (strcmp(value->valuestring, expected_value) != 0)
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	return true;
}

static bool bcd_expect_float_parameter(
	const cJSON *parameters,
	const char *name,
	float *out,
	bcd_check_result_t *result,
	const char *missing_code,
	const char *missing_message,
	const char *invalid_code,
	const char *invalid_message)
{
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, name);
	if (value == NULL)
	{
		bcd_set_result(result, false, missing_code, missing_message);
		return false;
	}

	if (!bcd_parse_float_value(value, out))
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	return true;
}

static bool bcd_parse_bool_value(const cJSON *value, bool *out)
{
	if (cJSON_IsBool(value))
	{
		*out = cJSON_IsTrue(value);
		return true;
	}

	if (!cJSON_IsString(value) || value->valuestring == NULL)
	{
		return false;
	}

	if (strcmp(value->valuestring, "true") == 0)
	{
		*out = true;
		return true;
	}

	if (strcmp(value->valuestring, "false") == 0)
	{
		*out = false;
		return true;
	}

	return false;
}

static bool bcd_expect_bool_parameter(
	const cJSON *parameters,
	const char *name,
	bool *out,
	bcd_check_result_t *result,
	const char *missing_code,
	const char *missing_message,
	const char *invalid_code,
	const char *invalid_message)
{
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, name);
	if (value == NULL)
	{
		bcd_set_result(result, false, missing_code, missing_message);
		return false;
	}

	if (!bcd_parse_bool_value(value, out))
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	return true;
}

static bool bcd_parse_point_object(
	const cJSON *point_json,
	point_t *out,
	bcd_check_result_t *result,
	const char *missing_code,
	const char *missing_message,
	const char *invalid_code,
	const char *invalid_message)
{
	if (!cJSON_IsObject(point_json))
	{
		bcd_set_result(result, false, missing_code, missing_message);
		return false;
	}

	const cJSON *x = cJSON_GetObjectItemCaseSensitive(point_json, "x");
	const cJSON *y = cJSON_GetObjectItemCaseSensitive(point_json, "y");
	if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	out->x = (float)x->valuedouble;
	out->y = (float)y->valuedouble;
	return true;
}

static bool bcd_validate_polygon_list_item(const cJSON *polygon_item)
{
	if (!cJSON_IsObject(polygon_item))
	{
		return false;
	}

	const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(polygon_item, "vertices");
	if (!cJSON_IsArray(vertices) || cJSON_GetArraySize(vertices) < 3)
	{
		return false;
	}

	int vertex_count = cJSON_GetArraySize(vertices);
	for (int i = 0; i < vertex_count; ++i)
	{
		const cJSON *point = cJSON_GetArrayItem(vertices, i);
		const cJSON *x = cJSON_GetObjectItemCaseSensitive(point, "x");
		const cJSON *y = cJSON_GetObjectItemCaseSensitive(point, "y");
		if (!cJSON_IsObject(point) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y))
		{
			return false;
		}
	}

	return true;
}

static bool bcd_validate_realworld_payload(const cJSON *root, bcd_check_result_t *result)
{
	const cJSON *realworld = cJSON_GetObjectItemCaseSensitive(root, "realworld");
	if (realworld == NULL)
	{
		return true;
	}

	if (!cJSON_IsObject(realworld))
	{
		bcd_set_result(result, false, "invalid_realworld", "realworld must be an object when provided.");
		return false;
	}

	const cJSON *zones = cJSON_GetObjectItemCaseSensitive(realworld, "zones");
	if (zones != NULL)
	{
		if (!cJSON_IsArray(zones))
		{
			bcd_set_result(result, false, "invalid_realworld_zones", "realworld.zones must be an array when provided.");
			return false;
		}

		int zone_count = cJSON_GetArraySize(zones);
		for (int i = 0; i < zone_count; ++i)
		{
			if (!bcd_validate_polygon_list_item(cJSON_GetArrayItem(zones, i)))
			{
				bcd_set_result(result, false, "invalid_realworld_zones", "realworld.zones must contain polygons with numeric vertices.");
				return false;
			}
		}
	}

	const cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(realworld, "obstacles");
	if (obstacles != NULL)
	{
		if (!cJSON_IsArray(obstacles))
		{
			bcd_set_result(result, false, "invalid_realworld_obstacles", "realworld.obstacles must be an array when provided.");
			return false;
		}

		int obstacle_count = cJSON_GetArraySize(obstacles);
		for (int i = 0; i < obstacle_count; ++i)
		{
			if (!bcd_validate_polygon_list_item(cJSON_GetArrayItem(obstacles, i)))
			{
				bcd_set_result(result, false, "invalid_realworld_obstacles", "realworld.obstacles must contain polygons with numeric vertices.");
				return false;
			}
		}
	}

	return true;
}

static int bcd_build_edges(polygon_t *polygon)
{
	if (polygon == NULL)
	{
		return -1;
	}

	if (polygon->edges != NULL)
	{
		va_free(polygon->edges);
		polygon->edges = NULL;
		polygon->edge_count = 0;
	}

	if (polygon->vertices == NULL || polygon->vertex_count < 2)
	{
		return -2;
	}

	polygon_edge_t *edges = (polygon_edge_t *)va_malloc((size_t)polygon->vertex_count * sizeof(polygon_edge_t));
	if (edges == NULL)
	{
		return -3;
	}

	for (uint32_t index = 0; index < polygon->vertex_count; ++index)
	{
		uint32_t next_index = (index + 1u) % polygon->vertex_count;
		edges[index].begin = polygon->vertices[index];
		edges[index].end = polygon->vertices[next_index];
	}

	polygon->edges = edges;
	polygon->edge_count = polygon->vertex_count;
	return 0;
}

static polygon_winding_t bcd_detect_winding(const point_t *vertices, uint32_t vertex_count)
{
	if (vertices == NULL || vertex_count < 3)
	{
		return POLYGON_WINDING_UNKNOWN;
	}

	double area2 = 0.0;
	for (uint32_t index = 0; index < vertex_count; ++index)
	{
		uint32_t next_index = (index + 1u) % vertex_count;
		area2 += (double)vertices[index].x * (double)vertices[next_index].y -
				 (double)vertices[index].y * (double)vertices[next_index].x;
	}

	// Screen coordinates (Y-down): positive signed area means clockwise order.
	if (area2 > 0.0)
	{
		return POLYGON_WINDING_CW;
	}
	if (area2 < 0.0)
	{
		return POLYGON_WINDING_CCW;
	}

	return POLYGON_WINDING_UNKNOWN;
}

static void bcd_reverse_vertices(point_t *vertices, uint32_t vertex_count)
{
	if (vertices == NULL || vertex_count < 2)
	{
		return;
	}

	for (uint32_t left = 0, right = vertex_count - 1; left < right; ++left, --right)
	{
		point_t tmp = vertices[left];
		vertices[left] = vertices[right];
		vertices[right] = tmp;
	}
}

static bool bcd_same_point(point_t a, point_t b)
{
	const float eps = 1e-6f;
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps;
}

static bool bcd_parse_polygon(
	const cJSON *polygon_json,
	polygon_t *polygon,
	polygon_winding_t winding,
	bcd_check_result_t *result,
	const char *invalid_code,
	const char *invalid_message)
{
	if (!cJSON_IsObject(polygon_json))
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(polygon_json, "vertices");
	if (!cJSON_IsArray(vertices))
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	int vertex_count = cJSON_GetArraySize(vertices);
	if (vertex_count < 3)
	{
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	point_t *parsed_vertices = (point_t *)va_malloc((size_t)vertex_count * sizeof(point_t));
	if (parsed_vertices == NULL)
	{
		bcd_set_result(result, false, "allocation_failed", "Failed to allocate polygon vertices.");
		return false;
	}

	for (int index = 0; index < vertex_count; ++index)
	{
		const cJSON *point = cJSON_GetArrayItem(vertices, index);
		const cJSON *x = cJSON_GetObjectItemCaseSensitive(point, "x");
		const cJSON *y = cJSON_GetObjectItemCaseSensitive(point, "y");
		if (!cJSON_IsObject(point) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y))
		{
			va_free(parsed_vertices);
			bcd_set_result(result, false, invalid_code, invalid_message);
			return false;
		}

		parsed_vertices[index].x = (float)x->valuedouble;
		parsed_vertices[index].y = (float)y->valuedouble;
	}

	// Compact consecutive duplicate vertices to avoid degenerate zero-length
	// edges that can break BCD event/cell assumptions downstream.
	uint32_t compact_count = 0;
	for (int index = 0; index < vertex_count; ++index)
	{
		point_t p = parsed_vertices[index];
		if (compact_count == 0 || !bcd_same_point(parsed_vertices[compact_count - 1], p))
		{
			parsed_vertices[compact_count++] = p;
		}
	}

	// Drop duplicated closing point if first==last after compaction.
	if (compact_count > 1 && bcd_same_point(parsed_vertices[0], parsed_vertices[compact_count - 1]))
	{
		compact_count--;
	}

	if (compact_count < 3)
	{
		va_free(parsed_vertices);
		bcd_set_result(result, false, invalid_code, invalid_message);
		return false;
	}

	polygon_winding_t detected_winding = bcd_detect_winding(parsed_vertices, compact_count);
	if (detected_winding != POLYGON_WINDING_UNKNOWN && detected_winding != winding)
	{
		bcd_reverse_vertices(parsed_vertices, compact_count);
	}

	polygon->winding = winding;
	polygon->vertices = parsed_vertices;
	polygon->vertex_count = compact_count;
	polygon->edges = NULL;
	polygon->edge_count = 0;

	if (bcd_build_edges(polygon) != 0)
	{
		va_free(parsed_vertices);
		polygon->vertices = NULL;
		polygon->vertex_count = 0;
		bcd_set_result(result, false, "allocation_failed", "Failed to allocate polygon edges.");
		return false;
	}

	return true;
}

void free_polygon(polygon_t *polygon)
{
	if (polygon == NULL)
	{
		return;
	}

	if (polygon->vertices != NULL)
	{
		va_free(polygon->vertices);
		polygon->vertices = NULL;
	}

	if (polygon->edges != NULL)
	{
		va_free(polygon->edges);
		polygon->edges = NULL;
	}

	polygon->vertex_count = 0;
	polygon->edge_count = 0;
	polygon->winding = POLYGON_WINDING_UNKNOWN;
}

void free_input_environment(input_environment_t *environment)
{
	if (environment == NULL)
	{
		return;
	}

	free_polygon(&environment->boundary);

	if (environment->obstacles != NULL)
	{
		for (uint32_t index = 0; index < environment->obstacle_count; ++index)
		{
			free_polygon(&environment->obstacles[index]);
		}

		va_free(environment->obstacles);
		environment->obstacles = NULL;
	}

	environment->obstacle_count = 0;
	free_polygon(&environment->realworld_boundary);
	if (environment->realworld_obstacles != NULL)
	{
		for (uint32_t index = 0; index < environment->realworld_obstacle_count; ++index)
		{
			free_polygon(&environment->realworld_obstacles[index]);
		}
		va_free(environment->realworld_obstacles);
		environment->realworld_obstacles = NULL;
	}
	environment->realworld_obstacle_count = 0;
	environment->path_width = 0.0f;
	environment->path_overlap = 0.0f;
	environment->id = 0;
}

bool bcd_check_request_json(const char *request_json, input_environment_t *environment, bcd_check_result_t *result)
{
	if (environment == NULL)
	{
		bcd_set_result(result, false, "invalid_request", "BCD request destination is required.");
		return false;
	}

	bcd_init_environment(environment);
	bcd_set_result(result, false, "invalid_request", "Invalid BCD compute request.");

	if (request_json == NULL)
	{
		bcd_set_result(result, false, "invalid_request", "BCD compute request JSON is required.");
		return false;
	}

	cJSON *root = cJSON_Parse(request_json);
	if (!cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_request", "BCD compute request must be a JSON object.");
		return false;
	}

	const cJSON *environment_json = cJSON_GetObjectItemCaseSensitive(root, "environment");
	if (!cJSON_IsObject(environment_json))
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "missing_environment", "BCD requires an environment object.");
		return false;
	}

	const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
	if (!cJSON_IsObject(parameters))
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "missing_parameters", "BCD requires a parameters object.");
		return false;
	}

	if (!bcd_validate_realworld_payload(root, result))
	{
		cJSON_Delete(root);
		return false;
	}

	const cJSON *zones = cJSON_GetObjectItemCaseSensitive(environment_json, "zones");
	if (!cJSON_IsArray(zones))
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_zones", "BCD requires zones to be an array with exactly one boundary polygon.");
		return false;
	}

	int zone_count = cJSON_GetArraySize(zones);
	if (zone_count != 1)
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_zone_count", "BCD requires exactly one boundary polygon in environment.zones.");
		return false;
	}

	if (!bcd_parse_polygon(
			cJSON_GetArrayItem(zones, 0),
			&environment->boundary,
			POLYGON_WINDING_CW,
			result,
			"invalid_boundary",
			"BCD boundary must be a polygon with at least 3 numeric vertices."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	const cJSON *start_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "startPoint");
	if (!bcd_parse_point_object(
			start_point_json,
			&environment->start_point,
			result,
			"missing_start_point",
			"BCD requires a startPoint in the environment.",
			"invalid_start_point",
			"BCD startPoint must be an object with numeric x and y."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	const cJSON *end_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "endPoint");
	if (!bcd_parse_point_object(
			end_point_json,
			&environment->end_point,
			result,
			"missing_end_point",
			"BCD requires an endPoint in the environment.",
			"invalid_end_point",
			"BCD endPoint must be an object with numeric x and y."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	const cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(environment_json, "obstacles");
	if (obstacles != NULL && !cJSON_IsArray(obstacles))
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_obstacles", "BCD obstacles must be an array when provided.");
		free_input_environment(environment);
		return false;
	}

	int obstacle_count = cJSON_IsArray(obstacles) ? cJSON_GetArraySize(obstacles) : 0;
	if (obstacle_count > 0)
	{
		environment->obstacles = (polygon_t *)va_calloc((size_t)obstacle_count, sizeof(polygon_t));
		if (environment->obstacles == NULL)
		{
			cJSON_Delete(root);
			bcd_set_result(result, false, "allocation_failed", "Failed to allocate BCD obstacles.");
			free_input_environment(environment);
			return false;
		}

		environment->obstacle_count = (uint32_t)obstacle_count;
		for (int index = 0; index < obstacle_count; ++index)
		{
			if (!bcd_parse_polygon(
					cJSON_GetArrayItem(obstacles, index),
					&environment->obstacles[index],
					POLYGON_WINDING_CCW,
					result,
					"invalid_obstacle",
					"BCD obstacles must be polygons with at least 3 numeric vertices."))
			{
				cJSON_Delete(root);
				free_input_environment(environment);
				return false;
			}
		}
	}

	if (!bcd_expect_float_parameter(
			parameters,
			"Path Width",
			&environment->path_width,
			result,
			"missing_path_width",
			"BCD requires a Path Width parameter.",
			"invalid_path_width",
			"BCD Path Width must be a number greater than 0."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	if (environment->path_width <= 0.0f)
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_path_width", "BCD Path Width must be greater than 0.");
		free_input_environment(environment);
		return false;
	}

	if (!bcd_expect_float_parameter(
			parameters,
			"Path Overlap",
			&environment->path_overlap,
			result,
			"missing_path_overlap",
			"BCD requires a Path Overlap parameter.",
			"invalid_path_overlap",
			"BCD Path Overlap must be a number greater than or equal to 0 and smaller than Path Width."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	if (environment->path_overlap < 0.0f || environment->path_overlap >= environment->path_width)
	{
		cJSON_Delete(root);
		bcd_set_result(result, false, "invalid_path_overlap", "BCD Path Overlap must be greater than or equal to 0 and smaller than Path Width.");
		free_input_environment(environment);
		return false;
	}

	if (!bcd_expect_string_parameter(
			parameters,
			"Format",
			"Polygon",
			result,
			"missing_format",
			"BCD requires a Format parameter.",
			"unsupported_format",
			"BCD supports only the Polygon format."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	if (!bcd_expect_string_parameter(
			parameters,
			"Type",
			"Off-Line",
			result,
			"missing_type",
			"BCD requires a Type parameter.",
			"unsupported_type",
			"BCD supports only the Off-Line type."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	if (!bcd_expect_string_parameter(
			parameters,
			"Coordinate System",
			"Cartesian",
			result,
			"missing_coordinate_system",
			"BCD requires a Coordinate System parameter.",
			"unsupported_coordinate_system",
			"BCD supports only the Cartesian coordinate system."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	bool track_memory_usage = false;
	if (!bcd_expect_bool_parameter(
			parameters,
			"Track Memory Usage",
			&track_memory_usage,
			result,
			"missing_track_memory_usage",
			"BCD requires a Track Memory Usage parameter.",
			"invalid_track_memory_usage",
			"BCD Track Memory Usage must be a boolean."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	environment->track_memory_usage = track_memory_usage;

	bool track_processing_time = false;
	if (!bcd_expect_bool_parameter(
			parameters,
			"Track Processing Time",
			&track_processing_time,
			result,
			"missing_track_processing_time",
			"BCD requires a Track Processing Time parameter.",
			"invalid_track_processing_time",
			"BCD Track Processing Time must be a boolean."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	environment->track_processing_time = track_processing_time;

	bool headland = false;
	if (!bcd_expect_bool_parameter(
			parameters,
			"Headland",
			&headland,
			result,
			"missing_headland",
			"BCD requires a Headland parameter.",
			"invalid_headland",
			"BCD Headland must be a boolean."))
	{
		cJSON_Delete(root);
		free_input_environment(environment);
		return false;
	}

	environment->headland = headland;

	cJSON_Delete(root);
	bcd_set_result(result, true, NULL, NULL);
	return true;
}
