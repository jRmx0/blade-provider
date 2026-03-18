/**
 * bcd_metadata.c
 *
 * Provides static descriptive information about the BCD algorithm.
 * Returns algorithm name, expected input
 * format, and produced output format.
 * Called by bcd.c in response to metadata requests from dispatcher.
 *
 * Dependencies: ../internal.h
 */

#include "../internal.h"
#include "../../metadata/metadata_json.h"

char *bcd_build_metadata_json(void)
{
	const char *format_values[] = {
		metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON)
	};
	const char *type_values[] = {
		metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE)
	};
	const char *coordsystem_values[] = {
		metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL)
	};

	cJSON *algorithm = cJSON_CreateObject();
	if (algorithm == NULL)
	{
		return NULL;
	}

	cJSON_AddNumberToObject(algorithm, "id", 1);
	cJSON_AddStringToObject(algorithm, "name", "Boustrophedon Cellular Decomposition");

	cJSON *parameters = cJSON_CreateArray();
	if (parameters == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}

	cJSON_AddItemToObject(algorithm, "parameters", parameters);

	metadata_add_parameter(parameters, 1, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, "15", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 2, BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, "Path Overlap", BCD_METADATA_PARAM_TYPE_DECIMAL, "5", NULL, 0, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 3, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Format", BCD_METADATA_PARAM_TYPE_ENUM, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), format_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT);
	metadata_add_parameter(parameters, 4, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Type", BCD_METADATA_PARAM_TYPE_ENUM, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), type_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE);
	metadata_add_parameter(parameters, 5, BCD_METADATA_PARAM_SECTION_ENVIRONMENT, "Coordinate System", BCD_METADATA_PARAM_TYPE_ENUM, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), coordsystem_values, 1, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM);

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
