/**
 * mock_algo_metadata.c
 *
 * Builds a metadata payload that intentionally exercises the supported
 * parameter contract for external client testing.
 */

#include "../../metadata/metadata_json.h"

char *mock_algo_build_metadata_json(void)
{
	const char *route_profile_values[] = {
		"Fast",
		"Balanced",
		"Precise",
		"Survey"
	};
	const char *edge_policy_values[] = {
		"Clip"
	};
	const char *format_locked_values[] = {
		metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON)
	};
	const char *format_multi_values[] = {
		metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON),
		metadata_format_to_string(BCD_METADATA_FORMAT_GRID)
	};
	const char *type_locked_values[] = {
		metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE)
	};
	const char *type_multi_values[] = {
		metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE),
		metadata_type_to_string(BCD_METADATA_TYPE_ONLINE)
	};
	const char *coordsystem_locked_values[] = {
		metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL)
	};
	const char *coordsystem_multi_values[] = {
		metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL),
		metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_LATLONG)
	};

	cJSON *algorithm = cJSON_CreateObject();
	if (algorithm == NULL)
	{
		return NULL;
	}

	cJSON_AddNumberToObject(algorithm, "id", 2);
	cJSON_AddStringToObject(algorithm, "name", "Mock Metadata Matrix");

	cJSON *parameters = cJSON_CreateArray();
	if (parameters == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}

	cJSON_AddItemToObject(algorithm, "parameters", parameters);

	metadata_add_parameter(parameters, 1, "Max Swaths", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, 0, "12", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 2, "Retry Limit", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, 0, NULL, BCD_METADATA_PARAM_SECTION_EXECUTION, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 3, "Row Spacing", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "1.25", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 4, "Turn Padding", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, NULL, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 5, "Allow Reverse", BCD_METADATA_PARAM_TYPE_BOOLEAN, NULL, 0, "True", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 6, "Emit Debug Trace", BCD_METADATA_PARAM_TYPE_BOOLEAN, NULL, 0, NULL, BCD_METADATA_PARAM_SECTION_EXECUTION, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 7, "Mission Code", BCD_METADATA_PARAM_TYPE_STRING, NULL, 0, "FIELD-ALPHA", BCD_METADATA_PARAM_SECTION_OBJECT, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 8, "Operator Notes", BCD_METADATA_PARAM_TYPE_STRING, NULL, 0, NULL, BCD_METADATA_PARAM_SECTION_OBJECT, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 9, "Route Profile", BCD_METADATA_PARAM_TYPE_ENUM, route_profile_values, 4, "Balanced", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 10, "Edge Policy", BCD_METADATA_PARAM_TYPE_ENUM, edge_policy_values, 1, NULL, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);
	metadata_add_parameter(parameters, 11, "Input Format (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, format_locked_values, 1, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT, 0, 0.0);
	metadata_add_parameter(parameters, 12, "Output Format Choices", BCD_METADATA_PARAM_TYPE_ENUM, format_multi_values, 2, NULL, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT, 0, 0.0);
	metadata_add_parameter(parameters, 13, "Execution Type (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, type_locked_values, 1, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE, 0, 0.0);
	metadata_add_parameter(parameters, 14, "Execution Type Choices", BCD_METADATA_PARAM_TYPE_ENUM, type_multi_values, 2, NULL, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE, 0, 0.0);
	metadata_add_parameter(parameters, 15, "Coordinate System (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, coordsystem_locked_values, 1, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM, 0, 0.0);
	metadata_add_parameter(parameters, 16, "Coordinate System Choices", BCD_METADATA_PARAM_TYPE_ENUM, coordsystem_multi_values, 2, NULL, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM, 0, 0.0);

	cJSON *layers = cJSON_CreateArray();
	if (layers == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}
	cJSON_AddItemToObject(algorithm, "layers", layers);

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
