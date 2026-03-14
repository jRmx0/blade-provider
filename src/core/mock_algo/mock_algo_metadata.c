/**
 * mock_algo_metadata.c
 *
 * Builds a metadata payload that intentionally exercises the supported
 * parameter contract for external client testing.
 */

#include "../metadata/metadata_json.h"

char *mock_algo_build_metadata_json(void)
{
	const char *route_profile_values[] = {
		"fast",
		"balanced",
		"precise",
		"survey"
	};
	const char *edge_policy_values[] = {
		"clip"
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

	cJSON_AddStringToObject(algorithm, "name", "mock_metadata_matrix");
	cJSON_AddStringToObject(algorithm, "label", "Mock Metadata Matrix");

	cJSON *parameters = cJSON_CreateArray();
	if (parameters == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}

	cJSON_AddItemToObject(algorithm, "parameters", parameters);

	metadata_add_parameter(parameters, "max_swaths", "Max Swaths", BCD_METADATA_PARAM_TYPE_INTEGER, "12", NULL, 0);
	metadata_add_parameter(parameters, "retry_limit", "Retry Limit", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, NULL, 0);
	metadata_add_parameter(parameters, "row_spacing", "Row Spacing", BCD_METADATA_PARAM_TYPE_DECIMAL, "1.25", NULL, 0);
	metadata_add_parameter(parameters, "turn_padding", "Turn Padding", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, NULL, 0);
	metadata_add_parameter(parameters, "allow_reverse", "Allow Reverse", BCD_METADATA_PARAM_TYPE_BOOLEAN, "true", NULL, 0);
	metadata_add_parameter(parameters, "emit_debug_trace", "Emit Debug Trace", BCD_METADATA_PARAM_TYPE_BOOLEAN, NULL, NULL, 0);
	metadata_add_parameter(parameters, "mission_code", "Mission Code", BCD_METADATA_PARAM_TYPE_STRING, "FIELD-ALPHA", NULL, 0);
	metadata_add_parameter(parameters, "operator_notes", "Operator Notes", BCD_METADATA_PARAM_TYPE_STRING, NULL, NULL, 0);
	metadata_add_parameter(parameters, "route_profile", "Route Profile", BCD_METADATA_PARAM_TYPE_ENUM, "balanced", route_profile_values, 4);
	metadata_add_parameter(parameters, "edge_policy", "Edge Policy", BCD_METADATA_PARAM_TYPE_ENUM, NULL, edge_policy_values, 1);
	metadata_add_parameter(parameters, "input_format_locked", "Input Format (Locked)", BCD_METADATA_PARAM_TYPE_FORMAT, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), format_locked_values, 1);
	metadata_add_parameter(parameters, "output_format_choices", "Output Format Choices", BCD_METADATA_PARAM_TYPE_FORMAT, NULL, format_multi_values, 2);
	metadata_add_parameter(parameters, "execution_type_locked", "Execution Type (Locked)", BCD_METADATA_PARAM_TYPE_TYPE, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), type_locked_values, 1);
	metadata_add_parameter(parameters, "execution_type_choices", "Execution Type Choices", BCD_METADATA_PARAM_TYPE_TYPE, NULL, type_multi_values, 2);
	metadata_add_parameter(parameters, "coordinate_system_locked", "Coordinate System (Locked)", BCD_METADATA_PARAM_TYPE_COORDSYSTEM, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), coordsystem_locked_values, 1);
	metadata_add_parameter(parameters, "coordinate_system_choices", "Coordinate System Choices", BCD_METADATA_PARAM_TYPE_COORDSYSTEM, NULL, coordsystem_multi_values, 2);

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}