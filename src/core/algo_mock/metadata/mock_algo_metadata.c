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

	cJSON_AddStringToObject(algorithm, "name", "Mock Metadata Matrix");

	cJSON *parameters = cJSON_CreateArray();
	if (parameters == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}

	cJSON_AddItemToObject(algorithm, "parameters", parameters);

	metadata_add_parameter(parameters, 1, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Max Swaths", BCD_METADATA_PARAM_TYPE_INTEGER, "12", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 2, BCD_METADATA_PARAM_SECTION_EXECUTION, "Retry Limit", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 3, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Row Spacing", BCD_METADATA_PARAM_TYPE_DECIMAL, "1.25", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 4, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Turn Padding", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 5, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Allow Reverse", BCD_METADATA_PARAM_TYPE_BOOLEAN, "True", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 6, BCD_METADATA_PARAM_SECTION_EXECUTION, "Emit Debug Trace", BCD_METADATA_PARAM_TYPE_BOOLEAN, NULL, NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 7, BCD_METADATA_PARAM_SECTION_OBJECT, "Mission Code", BCD_METADATA_PARAM_TYPE_STRING, "FIELD-ALPHA", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 8, BCD_METADATA_PARAM_SECTION_OBJECT, "Operator Notes", BCD_METADATA_PARAM_TYPE_STRING, NULL, NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 9, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Route Profile", BCD_METADATA_PARAM_TYPE_ENUM, "Balanced", route_profile_values, 4, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 10, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Edge Policy", BCD_METADATA_PARAM_TYPE_ENUM, NULL, edge_policy_values, 1, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 11, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Input Format (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), format_locked_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT);
	metadata_add_parameter(parameters, 12, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Output Format Choices", BCD_METADATA_PARAM_TYPE_ENUM, NULL, format_multi_values, 2, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT);
	metadata_add_parameter(parameters, 13, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Execution Type (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), type_locked_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE);
	metadata_add_parameter(parameters, 14, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Execution Type Choices", BCD_METADATA_PARAM_TYPE_ENUM, NULL, type_multi_values, 2, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE);
	metadata_add_parameter(parameters, 15, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Coordinate System (Locked)", BCD_METADATA_PARAM_TYPE_ENUM, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), coordsystem_locked_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM);
	metadata_add_parameter(parameters, 16, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Coordinate System Choices", BCD_METADATA_PARAM_TYPE_ENUM, NULL, coordsystem_multi_values, 2, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM);

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
