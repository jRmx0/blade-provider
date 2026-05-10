#include <ctype.h>
#include <string.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../../../dependencies/allocator/allocator.h"
#include "../internal.h"
#include "bounce_check.h"

static void bounce_init_environment(input_environment_t *environment)
{
	if (environment != NULL)
	{
		environment->id = 0;
		environment->path_width = 0.0f;
		environment->coverage_grid_cell_size = 0.0f;
		environment->bounce_offset = 0.0f;
		environment->target_coverage = 0.0f;
		environment->target_distance = 0.0f;
		environment->max_iterations = 0u;
		environment->track_memory_usage = false;
		environment->start_point.x = 0.0f;
		environment->start_point.y = 0.0f;
		environment->end_point.x = 0.0f;
		environment->end_point.y = 0.0f;
		environment->boundary.winding = POLYGON_WINDING_UNKNOWN;
		environment->boundary.vertices = NULL;
		environment->boundary.vertex_count = 0;
		environment->boundary.edges = NULL;
		environment->boundary.edge_count = 0;
		environment->obstacles = NULL;
		environment->obstacle_count = 0;
		environment->realworld_boundary.winding = POLYGON_WINDING_UNKNOWN;
		environment->realworld_boundary.vertices = NULL;
		environment->realworld_boundary.vertex_count = 0;
		environment->realworld_boundary.edges = NULL;
		environment->realworld_boundary.edge_count = 0;
		environment->realworld_obstacles = NULL;
		environment->realworld_obstacle_count = 0;
	}
}

static void bounce_init_polygon(polygon_t *polygon)
{
	if (polygon == NULL)
	{
		return;
	}

	polygon->winding = POLYGON_WINDING_UNKNOWN;
	polygon->vertices = NULL;
	polygon->vertex_count = 0;
	polygon->edges = NULL;
	polygon->edge_count = 0;
}

static bool bounce_build_polygon_edges(polygon_t *polygon)
{
	if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 3)
	{
		return false;
	}

	polygon_edge_t *edges = (polygon_edge_t *)va_malloc((size_t)polygon->vertex_count * sizeof(polygon_edge_t));
	if (edges == NULL)
	{
		return false;
	}

	for (uint32_t i = 0; i < polygon->vertex_count; ++i)
	{
		uint32_t next = (i + 1u) % polygon->vertex_count;
		edges[i].begin = polygon->vertices[i];
		edges[i].end = polygon->vertices[next];
	}

	polygon->edges = edges;
	polygon->edge_count = polygon->vertex_count;
	return true;
}

static void bounce_set_result(bounce_check_result_t *result, bool ok, const char *code, const char *message)
{
	if (result != NULL)
	{
		result->ok = ok;
		result->code = code;
		result->message = message;
	}
}

static void bounce_parse_float_value(const cJSON *value_item, const char *field_name, float *output, bounce_check_result_t *result)
{
	if (value_item == NULL || !cJSON_IsNumber(value_item))
	{
		bounce_set_result(result, false, "invalid_number", field_name);
		return;
	}
	*output = (float)value_item->valuedouble;
}

static void bounce_expect_float_parameter(const cJSON *parameters, const char *name, float *output, bounce_check_result_t *result)
{
	cJSON *param = cJSON_GetObjectItemCaseSensitive(parameters, name);
	if (param == NULL)
	{
		bounce_set_result(result, false, "missing_parameter", name);
		return;
	}
	bounce_parse_float_value(param, name, output, result);
}

static void bounce_parse_bool_value(const cJSON *value_item, const char *field_name, bool *output, bounce_check_result_t *result)
{
	if (value_item == NULL || !cJSON_IsBool(value_item))
	{
		bounce_set_result(result, false, "invalid_boolean", field_name);
		return;
	}
	*output = cJSON_IsTrue(value_item);
}

static void bounce_expect_bool_parameter(const cJSON *parameters, const char *name, bool *output, bounce_check_result_t *result)
{
	cJSON *param = cJSON_GetObjectItemCaseSensitive(parameters, name);
	if (param == NULL)
	{
		bounce_set_result(result, false, "missing_parameter", name);
		return;
	}
	bounce_parse_bool_value(param, name, output, result);
}

static void bounce_parse_point_object(const cJSON *point_item, const char *field_name, point_t *output, bounce_check_result_t *result)
{
	if (point_item == NULL || !cJSON_IsObject(point_item))
	{
		bounce_set_result(result, false, "invalid_object", field_name);
		return;
	}

	cJSON *x_item = cJSON_GetObjectItemCaseSensitive(point_item, "x");
	cJSON *y_item = cJSON_GetObjectItemCaseSensitive(point_item, "y");

	if (x_item == NULL || !cJSON_IsNumber(x_item))
	{
		bounce_set_result(result, false, "invalid_point_x", field_name);
		return;
	}

	if (y_item == NULL || !cJSON_IsNumber(y_item))
	{
		bounce_set_result(result, false, "invalid_point_y", field_name);
		return;
	}

	output->x = (float)x_item->valuedouble;
	output->y = (float)y_item->valuedouble;
}

static bool bounce_parse_polygon_vertices(const cJSON *vertices_array, polygon_t *polygon)
{
	bounce_init_polygon(polygon);

	if (vertices_array == NULL || !cJSON_IsArray(vertices_array))
	{
		return false;
	}

	uint32_t count = (uint32_t)cJSON_GetArraySize(vertices_array);
	if (count < 3)
	{
		return false;
	}

	polygon->vertices = (point_t *)va_malloc((size_t)count * sizeof(point_t));
	if (polygon->vertices == NULL)
	{
		return false;
	}

	polygon->vertex_count = count;
	for (uint32_t i = 0; i < count; i++)
	{
		cJSON *vertex_item = cJSON_GetArrayItem(vertices_array, (int)i);
		if (vertex_item == NULL || !cJSON_IsObject(vertex_item))
		{
			free_polygon(polygon);
			return false;
		}

		cJSON *x_item = cJSON_GetObjectItemCaseSensitive(vertex_item, "x");
		cJSON *y_item = cJSON_GetObjectItemCaseSensitive(vertex_item, "y");

		if (!cJSON_IsNumber(x_item) || !cJSON_IsNumber(y_item))
		{
			free_polygon(polygon);
			return false;
		}

		polygon->vertices[i].x = (float)x_item->valuedouble;
		polygon->vertices[i].y = (float)y_item->valuedouble;
	}

	if (!bounce_build_polygon_edges(polygon))
	{
		free_polygon(polygon);
		return false;
	}

	return true;
}

static bool bounce_validate_polygon_list_item(const cJSON *polygon_item)
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

	uint32_t count = (uint32_t)cJSON_GetArraySize(vertices);
	for (uint32_t i = 0; i < count; ++i)
	{
		const cJSON *point = cJSON_GetArrayItem(vertices, (int)i);
		const cJSON *x = cJSON_GetObjectItemCaseSensitive(point, "x");
		const cJSON *y = cJSON_GetObjectItemCaseSensitive(point, "y");
		if (!cJSON_IsObject(point) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y))
		{
			return false;
		}
	}

	return true;
}

static bool bounce_numbers_nearly_equal(double left, double right)
{
	double diff = left - right;
	if (diff < 0.0)
	{
		diff = -diff;
	}
	return diff <= 1e-6;
}

static bool bounce_vertices_array_equal(const cJSON *left_vertices, const cJSON *right_vertices)
{
	if (!cJSON_IsArray(left_vertices) || !cJSON_IsArray(right_vertices))
	{
		return false;
	}

	int left_count = cJSON_GetArraySize(left_vertices);
	int right_count = cJSON_GetArraySize(right_vertices);
	if (left_count != right_count)
	{
		return false;
	}

	for (int i = 0; i < left_count; ++i)
	{
		const cJSON *left_point = cJSON_GetArrayItem(left_vertices, i);
		const cJSON *right_point = cJSON_GetArrayItem(right_vertices, i);

		if (!cJSON_IsObject(left_point) || !cJSON_IsObject(right_point))
		{
			return false;
		}

		const cJSON *left_x = cJSON_GetObjectItemCaseSensitive(left_point, "x");
		const cJSON *left_y = cJSON_GetObjectItemCaseSensitive(left_point, "y");
		const cJSON *right_x = cJSON_GetObjectItemCaseSensitive(right_point, "x");
		const cJSON *right_y = cJSON_GetObjectItemCaseSensitive(right_point, "y");

		if (!cJSON_IsNumber(left_x) || !cJSON_IsNumber(left_y) || !cJSON_IsNumber(right_x) || !cJSON_IsNumber(right_y))
		{
			return false;
		}

		if (!bounce_numbers_nearly_equal(left_x->valuedouble, right_x->valuedouble) ||
			!bounce_numbers_nearly_equal(left_y->valuedouble, right_y->valuedouble))
		{
			return false;
		}
	}

	return true;
}

static bool bounce_polygon_array_equal(const cJSON *left_polygons, const cJSON *right_polygons)
{
	if (!cJSON_IsArray(left_polygons) || !cJSON_IsArray(right_polygons))
	{
		return false;
	}

	int left_count = cJSON_GetArraySize(left_polygons);
	int right_count = cJSON_GetArraySize(right_polygons);
	if (left_count != right_count)
	{
		return false;
	}

	for (int i = 0; i < left_count; ++i)
	{
		const cJSON *left_polygon = cJSON_GetArrayItem(left_polygons, i);
		const cJSON *right_polygon = cJSON_GetArrayItem(right_polygons, i);
		if (!cJSON_IsObject(left_polygon) || !cJSON_IsObject(right_polygon))
		{
			return false;
		}

		const cJSON *left_vertices = cJSON_GetObjectItemCaseSensitive(left_polygon, "vertices");
		const cJSON *right_vertices = cJSON_GetObjectItemCaseSensitive(right_polygon, "vertices");
		if (!bounce_vertices_array_equal(left_vertices, right_vertices))
		{
			return false;
		}
	}

	return true;
}

static bool bounce_validate_realworld_payload(const cJSON *root, bounce_check_result_t *result)
{
	cJSON *realworld = cJSON_GetObjectItemCaseSensitive(root, "realworld");
	if (realworld == NULL)
	{
		bounce_set_result(result, false, "missing_realworld", "Headland is required.");
		return false;
	}

	if (!cJSON_IsObject(realworld))
	{
		bounce_set_result(result, false, "invalid_realworld", "realworld must be an object when provided.");
		return false;
	}

	cJSON *zones = cJSON_GetObjectItemCaseSensitive(realworld, "zones");
	if (zones != NULL)
	{
		if (!cJSON_IsArray(zones))
		{
			bounce_set_result(result, false, "invalid_realworld_zones", "realworld.zones must be an array when provided.");
			return false;
		}

		uint32_t zone_count = (uint32_t)cJSON_GetArraySize(zones);
		for (uint32_t i = 0; i < zone_count; ++i)
		{
			if (!bounce_validate_polygon_list_item(cJSON_GetArrayItem(zones, (int)i)))
			{
				bounce_set_result(result, false, "invalid_realworld_zones", "realworld.zones must contain polygons with numeric vertices.");
				return false;
			}
		}
	}

	cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(realworld, "obstacles");
	if (obstacles != NULL)
	{
		if (!cJSON_IsArray(obstacles))
		{
			bounce_set_result(result, false, "invalid_realworld_obstacles", "realworld.obstacles must be an array when provided.");
			return false;
		}

		uint32_t obstacle_count = (uint32_t)cJSON_GetArraySize(obstacles);
		for (uint32_t i = 0; i < obstacle_count; ++i)
		{
			if (!bounce_validate_polygon_list_item(cJSON_GetArrayItem(obstacles, (int)i)))
			{
				bounce_set_result(result, false, "invalid_realworld_obstacles", "realworld.obstacles must contain polygons with numeric vertices.");
				return false;
			}
		}
	}

	cJSON *environment = cJSON_GetObjectItemCaseSensitive(root, "environment");
	if (cJSON_IsObject(environment))
	{
		const cJSON *env_zones = cJSON_GetObjectItemCaseSensitive(environment, "zones");
		const cJSON *env_obstacles = cJSON_GetObjectItemCaseSensitive(environment, "obstacles");

		if (cJSON_IsArray(env_zones) && cJSON_IsArray(zones) &&
			cJSON_IsArray(env_obstacles) && cJSON_IsArray(obstacles) &&
			bounce_polygon_array_equal(env_zones, zones) &&
			bounce_polygon_array_equal(env_obstacles, obstacles))
		{
			bounce_set_result(result, false, "invalid_headland_width", "Headland Width must be greater than 0 for Bounce.");
			return false;
		}
	}

	return true;
}

static void bounce_free_polygon_array(polygon_t *polygons, uint32_t polygon_count)
{
	if (polygons == NULL)
	{
		return;
	}

	for (uint32_t i = 0; i < polygon_count; ++i)
	{
		free_polygon(&polygons[i]);
	}
	va_free(polygons);
}

static bool bounce_parse_polygon_array(const cJSON *array,
									   polygon_t **polygons_out,
									   uint32_t *polygon_count_out)
{
	if (polygons_out == NULL || polygon_count_out == NULL)
	{
		return false;
	}

	*polygons_out = NULL;
	*polygon_count_out = 0;

	if (array == NULL)
	{
		return true;
	}

	if (!cJSON_IsArray(array))
	{
		return false;
	}

	uint32_t polygon_count = (uint32_t)cJSON_GetArraySize(array);
	if (polygon_count == 0)
	{
		return true;
	}

	polygon_t *polygons = (polygon_t *)va_calloc((size_t)polygon_count, sizeof(polygon_t));
	if (polygons == NULL)
	{
		return false;
	}

	for (uint32_t i = 0; i < polygon_count; ++i)
	{
		cJSON *polygon_item = cJSON_GetArrayItem(array, (int)i);
		if (polygon_item == NULL || !cJSON_IsObject(polygon_item))
		{
			bounce_free_polygon_array(polygons, i);
			return false;
		}

		cJSON *vertices_item = cJSON_GetObjectItemCaseSensitive(polygon_item, "vertices");
		if (vertices_item == NULL || !bounce_parse_polygon_vertices(vertices_item, &polygons[i]))
		{
			bounce_free_polygon_array(polygons, i);
			return false;
		}
	}

	*polygons_out = polygons;
	*polygon_count_out = polygon_count;
	return true;
}

static bool bounce_parse_realworld_payload(const cJSON *root, input_environment_t *environment, bounce_check_result_t *result)
{
	cJSON *realworld = cJSON_GetObjectItemCaseSensitive(root, "realworld");
	if (realworld == NULL)
	{
		bounce_set_result(result, false, "missing_realworld", "realworld is required.");
		return false;
	}

	if (!cJSON_IsObject(realworld))
	{
		bounce_set_result(result, false, "invalid_realworld", "realworld must be an object when provided.");
		return false;
	}

	environment->realworld_boundary.winding = POLYGON_WINDING_UNKNOWN;
	environment->realworld_boundary.vertices = NULL;
	environment->realworld_boundary.vertex_count = 0;
	environment->realworld_boundary.edges = NULL;
	environment->realworld_boundary.edge_count = 0;
	environment->realworld_obstacles = NULL;
	environment->realworld_obstacle_count = 0;

	cJSON *zones = cJSON_GetObjectItemCaseSensitive(realworld, "zones");
	if (zones != NULL)
	{
		if (!cJSON_IsArray(zones))
		{
			bounce_set_result(result, false, "invalid_realworld_zones", "realworld.zones must be an array when provided.");
			return false;
		}

		cJSON *first_zone = cJSON_GetArrayItem(zones, 0);
		if (first_zone == NULL || !cJSON_IsObject(first_zone))
		{
			bounce_set_result(result, false, "invalid_realworld_zones", "realworld.zones[0] must be an object when provided.");
			return false;
		}

		cJSON *vertices = cJSON_GetObjectItemCaseSensitive(first_zone, "vertices");
		if (vertices == NULL || !bounce_parse_polygon_vertices(vertices, &environment->realworld_boundary))
		{
			bounce_set_result(result, false, "invalid_realworld_zones", "realworld.zones[0].vertices must be a non-empty array.");
			return false;
		}
	}

	cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(realworld, "obstacles");
	if (obstacles != NULL)
	{
		if (!bounce_parse_polygon_array(obstacles, &environment->realworld_obstacles, &environment->realworld_obstacle_count))
		{
			free_polygon(&environment->realworld_boundary);
			bounce_free_polygon_array(environment->realworld_obstacles, environment->realworld_obstacle_count);
			environment->realworld_obstacles = NULL;
			environment->realworld_obstacle_count = 0;
			bounce_set_result(result, false, "invalid_realworld_obstacles", "realworld.obstacles must contain polygons with numeric vertices.");
			return false;
		}
	}

	return true;
}

bool bounce_check_request_json(const char *request_json, input_environment_t *environment, bounce_check_result_t *result)
{
	cJSON *root = NULL;
	bounce_init_environment(environment);
	bounce_set_result(result, true, NULL, NULL);

	if (request_json == NULL)
	{
		bounce_set_result(result, false, "null_request", "Request JSON cannot be null.");
		return false;
	}

	root = cJSON_Parse(request_json);
	if (root == NULL)
	{
		bounce_set_result(result, false, "invalid_json", "Failed to parse request JSON.");
		return false;
	}

	if (!bounce_validate_realworld_payload(root, result))
	{
		cJSON_Delete(root);
		return false;
	}

	// Extract the environment object (blade-terminal wraps all data under "environment")
	cJSON *env_obj = cJSON_GetObjectItemCaseSensitive(root, "environment");
	if (env_obj == NULL || !cJSON_IsObject(env_obj))
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "missing_environment", "environment field is required.");
		return false;
	}

	// Validate startPoint within environment
	cJSON *start_point = cJSON_GetObjectItemCaseSensitive(env_obj, "startPoint");
	if (start_point == NULL)
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "missing_startPoint", "startPoint field is required.");
		return false;
	}

	bounce_parse_point_object(start_point, "startPoint", &environment->start_point, result);
	if (!result->ok)
	{
		cJSON_Delete(root);
		return false;
	}

	// Validate zones (coverage area boundaries) - use first zone as boundary
	cJSON *zones = cJSON_GetObjectItemCaseSensitive(env_obj, "zones");
	if (zones == NULL || !cJSON_IsArray(zones))
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "missing_zones", "zones field is required as an array.");
		return false;
	}

	cJSON *first_zone = cJSON_GetArrayItem(zones, 0);
	if (first_zone == NULL || !cJSON_IsObject(first_zone))
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "invalid_zone", "zones[0] must be an object.");
		return false;
	}

	cJSON *boundary_vertices = cJSON_GetObjectItemCaseSensitive(first_zone, "vertices");
	if (boundary_vertices == NULL)
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "missing_boundary_vertices", "zones[0].vertices field is required.");
		return false;
	}

	if (!bounce_parse_polygon_vertices(boundary_vertices, &environment->boundary))
	{
		cJSON_Delete(root);
		bounce_set_result(result, false, "invalid_boundary_vertices", "zones[0].vertices must be a non-empty array.");
		return false;
	}

	// Validate obstacles
	cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(env_obj, "obstacles");
	if (obstacles == NULL || !cJSON_IsArray(obstacles))
	{
		cJSON_Delete(root);
		free_polygon(&environment->boundary);
		bounce_set_result(result, false, "missing_obstacles", "obstacles field is required as an array.");
		return false;
	}

	uint32_t obstacle_count = (uint32_t)cJSON_GetArraySize(obstacles);
	if (obstacle_count == 0)
	{
		environment->obstacles = NULL;
		environment->obstacle_count = 0;
	}
	else
	{
		environment->obstacles = (polygon_t *)va_calloc((size_t)obstacle_count, sizeof(polygon_t));
		if (environment->obstacles == NULL)
		{
			cJSON_Delete(root);
			free_polygon(&environment->boundary);
			bounce_set_result(result, false, "allocation_failed", "Failed to allocate obstacles array.");
			return false;
		}

		environment->obstacle_count = obstacle_count;

		for (uint32_t i = 0; i < obstacle_count; i++)
		{
			cJSON *obstacle_item = cJSON_GetArrayItem(obstacles, (int)i);
			if (obstacle_item == NULL || !cJSON_IsObject(obstacle_item))
			{
				cJSON_Delete(root);
				free_polygon(&environment->boundary);
				for (uint32_t j = 0; j < i; j++)
				{
					free_polygon(&environment->obstacles[j]);
				}
				va_free(environment->obstacles);
				environment->obstacles = NULL;
				environment->obstacle_count = 0;
				bounce_set_result(result, false, "invalid_obstacle", "obstacles[i] must be an object.");
				return false;
			}

			cJSON *vertices_item = cJSON_GetObjectItemCaseSensitive(obstacle_item, "vertices");
			if (vertices_item == NULL)
			{
				cJSON_Delete(root);
				free_polygon(&environment->boundary);
				for (uint32_t j = 0; j < i; j++)
				{
					free_polygon(&environment->obstacles[j]);
				}
				va_free(environment->obstacles);
				environment->obstacles = NULL;
				environment->obstacle_count = 0;
				bounce_set_result(result, false, "missing_obstacle_vertices", "obstacles[i].vertices field is required.");
				return false;
			}

			if (!bounce_parse_polygon_vertices(vertices_item, &environment->obstacles[i]))
			{
				cJSON_Delete(root);
				free_polygon(&environment->boundary);
				for (uint32_t j = 0; j <= i; j++)
				{
					free_polygon(&environment->obstacles[j]);
				}
				va_free(environment->obstacles);
				environment->obstacles = NULL;
				environment->obstacle_count = 0;
				bounce_set_result(result, false, "invalid_obstacle_vertices", "obstacles[i].vertices must be a non-empty array.");
				return false;
			}
		}
	}

	if (!bounce_parse_realworld_payload(root, environment, result))
	{
		cJSON_Delete(root);
		bounce_free_input_environment(environment);
		return false;
	}

	// Validate parameters (top-level, not in environment)
	cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
	if (parameters != NULL && cJSON_IsObject(parameters))
	{
		// Path Width parameter (in meters, will be used for coverage spacing)
		cJSON *path_width = cJSON_GetObjectItemCaseSensitive(parameters, "Path Width");
		if (path_width == NULL)
			path_width = cJSON_GetObjectItemCaseSensitive(parameters, "pathWidth");

		if (path_width != NULL && cJSON_IsNumber(path_width))
		{
			environment->path_width = (float)path_width->valuedouble;
		}
		else
		{
			environment->path_width = 15.0f;
		}
		// Target Coverage parameter (in %, 0-100)
		cJSON *target_coverage = cJSON_GetObjectItemCaseSensitive(parameters, "Target Coverage");
		if (target_coverage == NULL)
			target_coverage = cJSON_GetObjectItemCaseSensitive(parameters, "targetCoverage");

		if (target_coverage != NULL && cJSON_IsNumber(target_coverage))
		{
			float val = (float)target_coverage->valuedouble;
			if (val < 0.0f)
				val = 0.0f;
			if (val > 100.0f)
				val = 100.0f;
			environment->target_coverage = val;
		}
		else
		{
			environment->target_coverage = 0.0f;
		}

		// Target Distance parameter
		cJSON *target_distance = cJSON_GetObjectItemCaseSensitive(parameters, "Target Distance");
		if (target_distance == NULL)
			target_distance = cJSON_GetObjectItemCaseSensitive(parameters, "targetDistance");

		if (target_distance != NULL && cJSON_IsNumber(target_distance))
		{
			environment->target_distance = (float)target_distance->valuedouble;
		}
		else
		{
			environment->target_distance = 0.0f;
		}
		// Bounce Offset parameter (in %, 0-100)
		cJSON *bounce_offset = cJSON_GetObjectItemCaseSensitive(parameters, "Bounce Offset");
		if (bounce_offset == NULL)
			bounce_offset = cJSON_GetObjectItemCaseSensitive(parameters, "Random Bounce Offset");
		if (bounce_offset == NULL)
			bounce_offset = cJSON_GetObjectItemCaseSensitive(parameters, "randomBounceOffset");

		if (bounce_offset != NULL && cJSON_IsNumber(bounce_offset))
		{
			float val = (float)bounce_offset->valuedouble;
			if (val < 0.0f)
				val = 0.0f;
			if (val > 100.0f)
				val = 100.0f;
			environment->bounce_offset = val;
		}
		else
		{
			environment->bounce_offset = 0.0f;
		}

		// Starting Angle parameter (degrees [0, 360]; omit to pick randomly)
		cJSON *starting_angle_param = cJSON_GetObjectItemCaseSensitive(parameters, "Starting Angle");
		if (starting_angle_param == NULL)
			starting_angle_param = cJSON_GetObjectItemCaseSensitive(parameters, "startingAngle");
		if (starting_angle_param != NULL && cJSON_IsNumber(starting_angle_param))
		{
			float val = (float)starting_angle_param->valuedouble;
			if (val < 0.0f)
				val = 0.0f;
			if (val > 360.0f)
				val = 360.0f;
			environment->starting_angle = val;
		}
		else
		{
			environment->starting_angle = -1.0f; // random
		}

		// Max Iterations parameter (positive integer; 0 means "no explicit cap")
		cJSON *max_iterations_param = cJSON_GetObjectItemCaseSensitive(parameters, "Max Iterations");
		if (max_iterations_param == NULL)
			max_iterations_param = cJSON_GetObjectItemCaseSensitive(parameters, "maxIterations");
		if (max_iterations_param != NULL && cJSON_IsNumber(max_iterations_param))
		{
			double raw = max_iterations_param->valuedouble;
			if (raw > 0.0)
			{
				if (raw > 1000000.0)
					raw = 1000000.0;
				environment->max_iterations = (uint32_t)raw;
			}
			else
			{
				environment->max_iterations = 0u;
			}
		}
		else
		{
			environment->max_iterations = 0u;
		}

		// Coverage Grid Cell Size parameter (optional; <= 0 uses path_width/6 fallback)
		cJSON *coverage_grid_cell_size_param = cJSON_GetObjectItemCaseSensitive(parameters, "Coverage Grid Cell Size");
		if (coverage_grid_cell_size_param == NULL)
			coverage_grid_cell_size_param = cJSON_GetObjectItemCaseSensitive(parameters, "coverageGridCellSize");
		if (coverage_grid_cell_size_param != NULL && cJSON_IsNumber(coverage_grid_cell_size_param))
		{
			float val = (float)coverage_grid_cell_size_param->valuedouble;
			if (val < 0.0f)
				val = 0.0f;
			environment->coverage_grid_cell_size = val;
		}
		else
		{
			environment->coverage_grid_cell_size = 0.0f;
		}

		// Initialize other required fields
		environment->track_memory_usage = false;
	}
	else
	{
		// Use defaults if parameters not provided
		environment->path_width = 15.0f;
		environment->bounce_offset = 0.0f;
		environment->target_coverage = 0.0f;
		environment->target_distance = 0.0f;
		environment->max_iterations = 0u;
		environment->coverage_grid_cell_size = 0.0f;
		environment->starting_angle = -1.0f;
		environment->track_memory_usage = false;
	}

	// Validate: at least one stop target must be configured.
	// Multiple stop targets are allowed; processing stops when the first target is reached.
	if (environment->target_coverage < 0.001f &&
		environment->target_distance < 0.001f &&
		environment->max_iterations == 0u)
	{
		cJSON_Delete(root);
		free_polygon(&environment->boundary);
		if (environment->obstacles != NULL)
		{
			for (uint32_t i = 0; i < environment->obstacle_count; i++)
			{
				free_polygon(&environment->obstacles[i]);
			}
			va_free(environment->obstacles);
		}
		bounce_set_result(result, false, "invalid_parameters", "At least one stop target must be set: Target Coverage, Target Distance, or Max Iterations.");
		return false;
	}

	cJSON_Delete(root);
	bounce_set_result(result, true, NULL, NULL);
	return true;
}
