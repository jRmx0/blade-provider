#include <ctype.h>
#include <stdlib.h>

#include "cstar_check.h"
#include "../../../../dependencies/cJSON/cJSON.h"

static void cstar_set_result(cstar_check_result_t *result, bool ok, const char *code, const char *message)
{
    if (result == NULL)
    {
        return;
    }

    result->ok = ok;
    result->code = code;
    result->message = message;
}

static bool cstar_parse_float_value(const cJSON *value, float *out)
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

static bool cstar_parse_int_value(const cJSON *value, uint32_t *out)
{
    float parsed = 0.0f;
    if (!cstar_parse_float_value(value, &parsed))
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

static bool cstar_expect_float_parameter(
    const cJSON *parameters,
    const char *name,
    float *out,
    cstar_check_result_t *result,
    const char *missing_code,
    const char *missing_message,
    const char *invalid_code,
    const char *invalid_message)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, name);
    if (value == NULL)
    {
        cstar_set_result(result, false, missing_code, missing_message);
        return false;
    }

    if (!cstar_parse_float_value(value, out))
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    return true;
}

static bool cstar_expect_uint_parameter(
    const cJSON *parameters,
    const char *name,
    uint32_t *out,
    cstar_check_result_t *result,
    const char *missing_code,
    const char *missing_message,
    const char *invalid_code,
    const char *invalid_message)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, name);
    if (value == NULL)
    {
        cstar_set_result(result, false, missing_code, missing_message);
        return false;
    }

    if (!cstar_parse_int_value(value, out))
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    return true;
}

static bool cstar_validate_point_object(
    const cJSON *point_json,
    cstar_check_result_t *result,
    const char *missing_code,
    const char *missing_message,
    const char *invalid_code,
    const char *invalid_message)
{
    if (!cJSON_IsObject(point_json))
    {
        cstar_set_result(result, false, missing_code, missing_message);
        return false;
    }

    const cJSON *x = cJSON_GetObjectItemCaseSensitive(point_json, "x");
    const cJSON *y = cJSON_GetObjectItemCaseSensitive(point_json, "y");
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y))
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    return true;
}

static bool cstar_validate_polygon(
    const cJSON *polygon_json,
    cstar_check_result_t *result,
    const char *invalid_code,
    const char *invalid_message)
{
    if (!cJSON_IsObject(polygon_json))
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(polygon_json, "vertices");
    if (!cJSON_IsArray(vertices))
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    int vertex_count = cJSON_GetArraySize(vertices);
    if (vertex_count < 3)
    {
        cstar_set_result(result, false, invalid_code, invalid_message);
        return false;
    }

    for (int index = 0; index < vertex_count; ++index)
    {
        const cJSON *point = cJSON_GetArrayItem(vertices, index);
        const cJSON *x = cJSON_GetObjectItemCaseSensitive(point, "x");
        const cJSON *y = cJSON_GetObjectItemCaseSensitive(point, "y");
        if (!cJSON_IsObject(point) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y))
        {
            cstar_set_result(result, false, invalid_code, invalid_message);
            return false;
        }
    }

    return true;
}

bool cstar_validate_request_json(const char *request_json, cstar_check_result_t *result)
{
    cstar_set_result(result, false, "invalid_request", "Invalid C* compute request.");

    if (request_json == NULL)
    {
        cstar_set_result(result, false, "invalid_request", "C* compute request JSON is required.");
        return false;
    }

    cJSON *root = cJSON_Parse(request_json);
    if (!cJSON_IsObject(root))
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "invalid_request", "C* compute request must be a JSON object.");
        return false;
    }

    const cJSON *environment_json = cJSON_GetObjectItemCaseSensitive(root, "environment");
    if (!cJSON_IsObject(environment_json))
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "missing_environment", "C* requires an environment object.");
        return false;
    }

    const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
    if (!cJSON_IsObject(parameters))
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "missing_parameters", "C* requires a parameters object.");
        return false;
    }

    const cJSON *zones = cJSON_GetObjectItemCaseSensitive(environment_json, "zones");
    if (!cJSON_IsArray(zones) || cJSON_GetArraySize(zones) != 1)
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "invalid_zone_count", "C* requires exactly one boundary polygon in environment.zones.");
        return false;
    }

    if (!cstar_validate_polygon(
            cJSON_GetArrayItem(zones, 0),
            result,
            "invalid_boundary",
            "C* boundary must be a polygon with at least 3 numeric vertices."))
    {
        cJSON_Delete(root);
        return false;
    }

    const cJSON *start_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "startPoint");
    if (!cstar_validate_point_object(
            start_point_json,
            result,
            "missing_start_point",
            "C* requires a startPoint in the environment.",
            "invalid_start_point",
            "C* startPoint must be an object with numeric x and y."))
    {
        cJSON_Delete(root);
        return false;
    }

    const cJSON *end_point_json = cJSON_GetObjectItemCaseSensitive(environment_json, "endPoint");
    if (end_point_json != NULL)
    {
        if (!cstar_validate_point_object(
                end_point_json,
                result,
                "invalid_end_point",
                "C* endPoint must be an object with numeric x and y when provided.",
                "invalid_end_point",
                "C* endPoint must be an object with numeric x and y when provided."))
        {
            cJSON_Delete(root);
            return false;
        }
    }

    const cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(environment_json, "obstacles");
    if (obstacles != NULL && !cJSON_IsArray(obstacles))
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "invalid_obstacles", "C* obstacles must be an array when provided.");
        return false;
    }

    int obstacle_count = cJSON_IsArray(obstacles) ? cJSON_GetArraySize(obstacles) : 0;
    for (int index = 0; index < obstacle_count; ++index)
    {
        if (!cstar_validate_polygon(
                cJSON_GetArrayItem(obstacles, index),
                result,
                "invalid_obstacle",
                "C* obstacles must be polygons with at least 3 numeric vertices."))
        {
            cJSON_Delete(root);
            return false;
        }
    }

    float path_width = 0.0f;
    if (!cstar_expect_float_parameter(
            parameters,
            "Path Width",
            &path_width,
            result,
            "missing_path_width",
            "C* requires a Path Width parameter.",
            "invalid_path_width",
            "C* Path Width must be a number greater than 0."))
    {
        cJSON_Delete(root);
        return false;
    }

    if (path_width <= 0.0f)
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "invalid_path_width", "C* Path Width must be greater than 0.");
        return false;
    }

    uint32_t frontier_spacing_multiplier = 0u;
    if (!cstar_expect_uint_parameter(
            parameters,
            "Frontier Spacing Multiplier",
            &frontier_spacing_multiplier,
            result,
            "missing_frontier_spacing_multiplier",
            "C* requires a Frontier Spacing Multiplier parameter.",
            "invalid_frontier_spacing_multiplier",
            "C* Frontier Spacing Multiplier must be an integer greater than or equal to 1."))
    {
        cJSON_Delete(root);
        return false;
    }

    if (frontier_spacing_multiplier < 1u)
    {
        cJSON_Delete(root);
        cstar_set_result(result, false, "invalid_frontier_spacing_multiplier", "C* Frontier Spacing Multiplier must be greater than or equal to 1.");
        return false;
    }

    cJSON_Delete(root);
    cstar_set_result(result, true, NULL, NULL);
    return true;
}
