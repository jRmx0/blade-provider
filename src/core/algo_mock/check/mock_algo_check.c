#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mock_algo_check.h"
#include "../../../../dependencies/cJSON/cjson.h"

typedef enum
{
	MOCK_PARAM_TYPE_INTEGER = 0,
	MOCK_PARAM_TYPE_DECIMAL,
	MOCK_PARAM_TYPE_BOOLEAN,
	MOCK_PARAM_TYPE_STRING,
	MOCK_PARAM_TYPE_ENUM
} mock_param_type_t;

typedef struct
{
	const char *name;
	mock_param_type_t type;
	const char *const *enum_values;
	size_t enum_value_count;
} mock_param_spec_t;

static const char *ROUTE_PROFILE_VALUES[] = { "Fast", "Balanced", "Precise", "Survey" };
static const char *EDGE_POLICY_VALUES[] = { "Clip" };
static const char *FORMAT_LOCKED_VALUES[] = { "Polygon" };
static const char *FORMAT_MULTI_VALUES[] = { "Polygon", "Grid" };
static const char *TYPE_LOCKED_VALUES[] = { "Off-Line" };
static const char *TYPE_MULTI_VALUES[] = { "Off-Line", "On-Line" };
static const char *COORDSYSTEM_LOCKED_VALUES[] = { "Decimal" };
static const char *COORDSYSTEM_MULTI_VALUES[] = { "Decimal", "Lat/Long" };

static const mock_param_spec_t MOCK_PARAM_SPECS[] = {
	{ "Max Swaths", MOCK_PARAM_TYPE_INTEGER, NULL, 0 },
	{ "Retry Limit", MOCK_PARAM_TYPE_INTEGER, NULL, 0 },
	{ "Row Spacing", MOCK_PARAM_TYPE_DECIMAL, NULL, 0 },
	{ "Turn Padding", MOCK_PARAM_TYPE_DECIMAL, NULL, 0 },
	{ "Allow Reverse", MOCK_PARAM_TYPE_BOOLEAN, NULL, 0 },
	{ "Emit Debug Trace", MOCK_PARAM_TYPE_BOOLEAN, NULL, 0 },
	{ "Mission Code", MOCK_PARAM_TYPE_STRING, NULL, 0 },
	{ "Operator Notes", MOCK_PARAM_TYPE_STRING, NULL, 0 },
	{ "Route Profile", MOCK_PARAM_TYPE_ENUM, ROUTE_PROFILE_VALUES, sizeof(ROUTE_PROFILE_VALUES) / sizeof(ROUTE_PROFILE_VALUES[0]) },
	{ "Edge Policy", MOCK_PARAM_TYPE_ENUM, EDGE_POLICY_VALUES, sizeof(EDGE_POLICY_VALUES) / sizeof(EDGE_POLICY_VALUES[0]) },
	{ "Input Format (Locked)", MOCK_PARAM_TYPE_ENUM, FORMAT_LOCKED_VALUES, sizeof(FORMAT_LOCKED_VALUES) / sizeof(FORMAT_LOCKED_VALUES[0]) },
	{ "Output Format Choices", MOCK_PARAM_TYPE_ENUM, FORMAT_MULTI_VALUES, sizeof(FORMAT_MULTI_VALUES) / sizeof(FORMAT_MULTI_VALUES[0]) },
	{ "Execution Type (Locked)", MOCK_PARAM_TYPE_ENUM, TYPE_LOCKED_VALUES, sizeof(TYPE_LOCKED_VALUES) / sizeof(TYPE_LOCKED_VALUES[0]) },
	{ "Execution Type Choices", MOCK_PARAM_TYPE_ENUM, TYPE_MULTI_VALUES, sizeof(TYPE_MULTI_VALUES) / sizeof(TYPE_MULTI_VALUES[0]) },
	{ "Coordinate System (Locked)", MOCK_PARAM_TYPE_ENUM, COORDSYSTEM_LOCKED_VALUES, sizeof(COORDSYSTEM_LOCKED_VALUES) / sizeof(COORDSYSTEM_LOCKED_VALUES[0]) },
	{ "Coordinate System Choices", MOCK_PARAM_TYPE_ENUM, COORDSYSTEM_MULTI_VALUES, sizeof(COORDSYSTEM_MULTI_VALUES) / sizeof(COORDSYSTEM_MULTI_VALUES[0]) },
};

static void mock_set_result(mock_algo_check_result_t *result, bool ok, const char *code, const char *message)
{
	if (result == NULL)
	{
		return;
	}

	result->ok = ok;
	result->code = code;
	result->message = message;
}

static int mock_compare_ignore_case(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0')
	{
		int left_char = tolower((unsigned char)*left);
		int right_char = tolower((unsigned char)*right);
		if (left_char != right_char)
		{
			return left_char - right_char;
		}

		left++;
		right++;
	}

	return tolower((unsigned char)*left) - tolower((unsigned char)*right);
}

static const mock_param_spec_t *mock_find_param_spec(const char *name)
{
	for (size_t index = 0; index < sizeof(MOCK_PARAM_SPECS) / sizeof(MOCK_PARAM_SPECS[0]); ++index)
	{
		if (strcmp(MOCK_PARAM_SPECS[index].name, name) == 0)
		{
			return &MOCK_PARAM_SPECS[index];
		}
	}

	return NULL;
}

static bool mock_parse_integer(const cJSON *value)
{
	if (cJSON_IsNumber(value))
	{
		double integer_part = (double)((long long)value->valuedouble);
		return value->valuedouble == integer_part;
	}

	if (!cJSON_IsString(value) || value->valuestring == NULL)
	{
		return false;
	}

	char *end = NULL;
	strtoll(value->valuestring, &end, 10);
	if (value->valuestring == end)
	{
		return false;
	}

	while (end != NULL && *end != '\0' && isspace((unsigned char)*end))
	{
		end++;
	}

	return end != NULL && *end == '\0';
}

static bool mock_parse_decimal(const cJSON *value)
{
	if (cJSON_IsNumber(value))
	{
		return true;
	}

	if (!cJSON_IsString(value) || value->valuestring == NULL)
	{
		return false;
	}

	char *end = NULL;
	strtod(value->valuestring, &end);
	if (value->valuestring == end)
	{
		return false;
	}

	while (end != NULL && *end != '\0' && isspace((unsigned char)*end))
	{
		end++;
	}

	return end != NULL && *end == '\0';
}

static bool mock_parse_boolean(const cJSON *value)
{
	if (cJSON_IsBool(value))
	{
		return true;
	}

	if (!cJSON_IsString(value) || value->valuestring == NULL)
	{
		return false;
	}

	return mock_compare_ignore_case(value->valuestring, "true") == 0 || mock_compare_ignore_case(value->valuestring, "false") == 0;
}

static bool mock_parse_string(const cJSON *value)
{
	return cJSON_IsString(value) && value->valuestring != NULL && value->valuestring[0] != '\0';
}

static bool mock_parse_enum(const cJSON *value, const mock_param_spec_t *spec)
{
	if (!mock_parse_string(value))
	{
		return false;
	}

	for (size_t index = 0; index < spec->enum_value_count; ++index)
	{
		if (strcmp(value->valuestring, spec->enum_values[index]) == 0)
		{
			return true;
		}
	}

	return false;
}

static bool mock_validate_parameter_value(const cJSON *value, const mock_param_spec_t *spec)
{
	switch (spec->type)
	{
	case MOCK_PARAM_TYPE_INTEGER:
		return mock_parse_integer(value);
	case MOCK_PARAM_TYPE_DECIMAL:
		return mock_parse_decimal(value);
	case MOCK_PARAM_TYPE_BOOLEAN:
		return mock_parse_boolean(value);
	case MOCK_PARAM_TYPE_STRING:
		return mock_parse_string(value);
	case MOCK_PARAM_TYPE_ENUM:
		return mock_parse_enum(value, spec);
	default:
		return false;
	}
}

static bool mock_validate_polygon_object(const cJSON *polygon)
{
	if (!cJSON_IsObject(polygon))
	{
		return false;
	}

	const cJSON *vertices = cJSON_GetObjectItemCaseSensitive(polygon, "vertices");
	if (!cJSON_IsArray(vertices) || cJSON_GetArraySize(vertices) < 3)
	{
		return false;
	}

	int vertex_count = cJSON_GetArraySize(vertices);
	for (int index = 0; index < vertex_count; ++index)
	{
		const cJSON *point = cJSON_GetArrayItem(vertices, index);
		const cJSON *x = cJSON_GetObjectItemCaseSensitive(point, "x");
		const cJSON *y = cJSON_GetObjectItemCaseSensitive(point, "y");
		if (!cJSON_IsObject(point) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y))
		{
			return false;
		}
	}

	return true;
}

static bool mock_validate_polygon_collection(const cJSON *collection, bool require_at_least_one)
{
	if (!cJSON_IsArray(collection))
	{
		return false;
	}

	int count = cJSON_GetArraySize(collection);
	if (require_at_least_one && count < 1)
	{
		return false;
	}

	for (int index = 0; index < count; ++index)
	{
		if (!mock_validate_polygon_object(cJSON_GetArrayItem(collection, index)))
		{
			return false;
		}
	}

	return true;
}

bool mock_algo_check_request_json(const char *request_json, mock_algo_check_result_t *result)
{
	mock_set_result(result, false, "invalid_request", "Invalid mock compute request.");

	if (request_json == NULL)
	{
		mock_set_result(result, false, "invalid_request", "Mock algorithm request JSON is required.");
		return false;
	}

	cJSON *root = cJSON_Parse(request_json);
	if (!cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		mock_set_result(result, false, "invalid_request", "Mock algorithm request must be a JSON object.");
		return false;
	}

	const cJSON *environment = cJSON_GetObjectItemCaseSensitive(root, "environment");
	if (!cJSON_IsObject(environment))
	{
		cJSON_Delete(root);
		mock_set_result(result, false, "missing_environment", "Mock algorithm requires an environment object.");
		return false;
	}

	const cJSON *zones = cJSON_GetObjectItemCaseSensitive(environment, "zones");
	if (!mock_validate_polygon_collection(zones, true))
	{
		cJSON_Delete(root);
		mock_set_result(result, false, "invalid_zones", "Mock algorithm requires environment.zones to contain polygon objects with at least 3 numeric vertices.");
		return false;
	}

	const cJSON *obstacles = cJSON_GetObjectItemCaseSensitive(environment, "obstacles");
	if (obstacles != NULL && !mock_validate_polygon_collection(obstacles, false))
	{
		cJSON_Delete(root);
		mock_set_result(result, false, "invalid_obstacles", "Mock algorithm requires environment.obstacles to be an array of polygon objects when provided.");
		return false;
	}

	const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
	if (parameters != NULL && !cJSON_IsObject(parameters))
	{
		cJSON_Delete(root);
		mock_set_result(result, false, "invalid_parameters", "Mock algorithm parameters must be an object when provided.");
		return false;
	}

	if (cJSON_IsObject(parameters))
	{
		for (const cJSON *parameter = parameters->child; parameter != NULL; parameter = parameter->next)
		{
			if (parameter->string == NULL || parameter->string[0] == '\0')
			{
				cJSON_Delete(root);
				mock_set_result(result, false, "invalid_parameter", "Mock algorithm parameters must use string keys.");
				return false;
			}

			const mock_param_spec_t *spec = mock_find_param_spec(parameter->string);
			if (spec == NULL)
			{
				cJSON_Delete(root);
				mock_set_result(result, false, "unknown_parameter", "Mock algorithm received an unknown parameter.");
				return false;
			}

			if (!mock_validate_parameter_value(parameter, spec))
			{
				cJSON_Delete(root);
				mock_set_result(result, false, "invalid_parameter", "Mock algorithm received a parameter value that does not match its metadata contract.");
				return false;
			}
		}
	}

	cJSON_Delete(root);
	mock_set_result(result, true, NULL, NULL);
	return true;
}