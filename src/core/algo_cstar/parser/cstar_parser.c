#include <ctype.h>
#include <stdlib.h>

#include "cstar_parser.h"
#include "../../../../dependencies/cJSON/cJSON.h"
#include "../../../../dependencies/allocator/allocator.h"

static void cstar_parser_set_result(cstar_check_result_t *result, bool ok, const char *code, const char *message)
{
    if (result == NULL)
    {
        return;
    }

    result->ok = ok;
    result->code = code;
    result->message = message;
}

static void cstar_parser_init_environment(cstar_environment_t *environment)
{
    if (environment == NULL)
    {
        return;
    }

    environment->id = 0;
    environment->path_width = 0.0f;
    environment->sensor_range = 0.0f;
    environment->track_memory_usage = false;
    environment->headland = false;

    environment->start_point.x = 0.0f;
    environment->start_point.y = 0.0f;
    environment->end_point.x = 0.0f;
    environment->end_point.y = 0.0f;

    environment->operationalBoundary.winding = POLYGON_WINDING_UNKNOWN;
    environment->operationalBoundary.vertices = NULL;
    environment->operationalBoundary.vertex_count = 0;
    environment->operationalBoundary.edges = NULL;
    environment->operationalBoundary.edge_count = 0;

    environment->operationalObstacles = NULL;
    environment->obstacle_count = 0;
}

static bool cstar_parser_parse_float_value(const cJSON *value, float *out)
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

static bool cstar_parser_parse_int_value(const cJSON *value, uint32_t *out)
{
    float parsed = 0.0f;
    if (!cstar_parser_parse_float_value(value, &parsed))
    {
        return false;
    }

    if (parsed < 0.0f)
    {
        return false;
    }

    uint32_t as_int = (uint32_t)parsed;
    if ((float)as_int != parsed)
    {
        return false;
    }

    *out = as_int;
    return true;
}

static bool cstar_parser_parse_point_object(
    const cJSON *point_json,
    point_t *out,
    cstar_check_result_t *result,
    const char *error_code,
    const char *error_message)
{
    if (!cJSON_IsObject(point_json))
    {
        cstar_parser_set_result(result, false, error_code, error_message);
        return false;
    }

    const cJSON *x = cJSON_GetObjectItemCaseSensitive(point_json, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(point_json, "y");
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
    {
        cstar_parser_set_result(result, false, error_code, error_message);
        return false;
    }

    out->x = (float)x->valuedouble;
    out->y = (float)y->valuedouble;
    return true;
}

static int cstar_parser_build_edges(polygon_t *polygon)
{
    if (polygon == NULL)
    {
        return -1;
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

static bool cstar_parser_parse_polygon(
    const cJSON *polygon_json,
    polygon_t *polygon,
    polygon_winding_t winding,
    cstar_check_result_t *result,
    const char *invalid_code,
    const char *invalid_message)
{
    if (!cJSON_IsObject(polygon_json))
    {
        cstar_parser_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(polygon_json, "vertices");
    if (!cJSON_IsArray(vertices))
    {
        cstar_parser_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    int vertex_count = cJSON_GetArraySize(vertices);
    if (vertex_count < 3)
    {
        cstar_parser_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    point_t *parsed_vertices = (point_t *)va_malloc((size_t)vertex_count * sizeof(point_t));
    if (parsed_vertices == NULL)
    {
        cstar_parser_set_result(result, false, "allocation_failed", "Failed to allocate C* polygon vertices.");
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
            cstar_parser_set_result(result, false, invalid_code, invalid_message);
            return false;
        }

        parsed_vertices[index].x = (float)x->valuedouble;
        parsed_vertices[index].y = (float)y->valuedouble;
    }

    polygon->winding = winding;
    polygon->vertices = parsed_vertices;
    polygon->vertex_count = (uint32_t)vertex_count;
    polygon->edges = NULL;
    polygon->edge_count = 0;

    if (cstar_parser_build_edges(polygon) != 0)
    {
        va_free(parsed_vertices);
        polygon->vertices = NULL;
        polygon->vertex_count = 0;
        cstar_parser_set_result(result, false, "allocation_failed", "Failed to allocate C* polygon edges.");
        return false;
    }

    return true;
}

void cstar_parser_free_environment(cstar_environment_t *environment)
{
    if (environment == NULL)
    {
        return;
    }

    free_polygon(&environment->operationalBoundary);

    if (environment->operationalObstacles != NULL)
    {
        for (uint32_t i = 0; i < environment->obstacle_count; ++i)
        {
            free_polygon(&environment->operationalObstacles[i]);
        }
        free(environment->operationalObstacles);
        environment->operationalObstacles = NULL;
    }

    environment->obstacle_count = 0;
}

bool cstar_parse_request_json(const char *request_json, cstar_environment_t *environment, cstar_check_result_t *result)
{
    if (environment == NULL)
    {
        cstar_parser_set_result(result, false, "invalid_request", "C* request destination is required.");
        return false;
    }

    cstar_parser_init_environment(environment);
    cstar_parser_set_result(result, false, "invalid_request", "Invalid C* compute request.");

    if (request_json == NULL)
    {
        cstar_parser_set_result(result, false, "invalid_request", "C* compute request JSON is required.");
        return false;
    }

    cJSON *root = cJSON_Parse(request_json);
    if (!cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_request", "C* compute request must be a JSON object.");
        return false;
    }

    const cJSON *environment_json = cJSON_GetObjectItemCaseSensitive(root, "environment");
    const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
    if (!cJSON_IsObject(environment_json) || !cJSON_IsObject(parameters))
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_request", "C* compute request must include environment and parameters objects.");
        return false;
    }

    const cJSON *zones = cJSON_GetObjectItemCaseSensitive(environment_json, "zones");
    if (!cJSON_IsArray(zones) || cJSON_GetArraySize(zones) != 1)
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_zone_count", "C* requires exactly one boundary polygon in environment.zones.");
        return false;
    }

    if (!cstar_parser_parse_polygon(
            cJSON_GetArrayItem(zones, 0),
            &environment->operationalBoundary,
            POLYGON_WINDING_CW,
            result,
            "invalid_boundary",
            "C* boundary must be a polygon with at least 3 numeric vertices."))
    {
        cJSON_Delete(root);
        cstar_parser_free_environment(environment);
        return false;
    }

    const cJSON *start_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "startPoint");
    if (!cstar_parser_parse_point_object(
            start_point_json,
            &environment->start_point,
            result,
            "invalid_start_point",
            "C* startPoint must be an object with numeric x and y."))
    {
        cJSON_Delete(root);
        cstar_parser_free_environment(environment);
        return false;
    }

    const cJSON *end_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "endPoint");
    if (end_point_json != NULL)
    {
        if (!cstar_parser_parse_point_object(
                end_point_json,
                &environment->end_point,
                result,
                "invalid_end_point",
                "C* endPoint must be an object with numeric x and y when provided."))
        {
            cJSON_Delete(root);
            cstar_parser_free_environment(environment);
            return false;
        }
    }
    else
    {
        environment->end_point = environment->start_point;
    }

    const cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(environment_json, "obstacles");
    int obstacle_count = cJSON_IsArray(obstacles) ? cJSON_GetArraySize(obstacles) : 0;
    if (obstacle_count > 0)
    {
        environment->operationalObstacles = (polygon_t *)va_calloc((size_t)obstacle_count, sizeof(polygon_t));
        if (environment->operationalObstacles == NULL)
        {
            cJSON_Delete(root);
            cstar_parser_set_result(result, false, "allocation_failed", "Failed to allocate C* obstacles.");
            cstar_parser_free_environment(environment);
            return false;
        }

        environment->obstacle_count = (uint32_t)obstacle_count;
        for (int index = 0; index < obstacle_count; ++index)
        {
            if (!cstar_parser_parse_polygon(
                    cJSON_GetArrayItem(obstacles, index),
                    &environment->operationalObstacles[index],
                    POLYGON_WINDING_CCW,
                    result,
                    "invalid_obstacle",
                    "C* obstacles must be polygons with at least 3 numeric vertices."))
            {
                cJSON_Delete(root);
                cstar_parser_free_environment(environment);
                return false;
            }
        }
    }

    const cJSON *path_width_value = cJSON_GetObjectItemCaseSensitive(parameters, "Path Width");
    if (!cstar_parser_parse_float_value(path_width_value, &environment->path_width) || environment->path_width <= 0.0f)
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_path_width", "C* Path Width must be a number greater than 0.");
        cstar_parser_free_environment(environment);
        return false;
    }

    const cJSON *sensor_range_value = cJSON_GetObjectItemCaseSensitive(parameters, "Sensor Range");
    if (!cstar_parser_parse_float_value(sensor_range_value, &environment->sensor_range) || environment->sensor_range <= 0.0f)
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_sensor_range", "C* Sensor Range must be a number greater than 0.");
        cstar_parser_free_environment(environment);
        return false;
    }

    const cJSON *frontier_spacing_value = cJSON_GetObjectItemCaseSensitive(parameters, "Frontier Spacing Multiplier");
    uint32_t frontier_spacing_multiplier = 0u;
    if (!cstar_parser_parse_int_value(frontier_spacing_value, &frontier_spacing_multiplier) || frontier_spacing_multiplier < 1u)
    {
        cJSON_Delete(root);
        cstar_parser_set_result(result, false, "invalid_frontier_spacing", "C* Frontier Spacing Multiplier must be a positive integer.");
        cstar_parser_free_environment(environment);
        return false;
    }

    cJSON_Delete(root);
    cstar_parser_set_result(result, true, NULL, NULL);
    return true;
}
