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

	metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "15", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 2, "Path Overlap", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "5", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 3, "Format", BCD_METADATA_PARAM_TYPE_ENUM, format_values, 1, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT);
	metadata_add_parameter(parameters, 4, "Type", BCD_METADATA_PARAM_TYPE_ENUM, type_values, 1, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE);
	metadata_add_parameter(parameters, 5, "Coordinate System", BCD_METADATA_PARAM_TYPE_ENUM, coordsystem_values, 1, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM);

	cJSON *debug_layers = cJSON_CreateArray();
	if (debug_layers == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}
	cJSON_AddItemToObject(algorithm, "debugLayers", debug_layers);

	metadata_add_debug_layer(debug_layers, 1, "eventList",       "Event List",       BCD_METADATA_DEBUG_LAYER_TYPE_POINT,   "text-purple-500 stroke-purple-500",              4,  -1);
	metadata_add_debug_layer(debug_layers, 2, "cellList",        "Cell List",        BCD_METADATA_DEBUG_LAYER_TYPE_POLYGON, "text-blue-400 stroke-blue-400 fill-blue-400/10", -1, -1);
	metadata_add_debug_layer(debug_layers, 3, "cellVisitOrder", "Cell Visit Order", BCD_METADATA_DEBUG_LAYER_TYPE_POINT,   "text-yellow-300",                                -1,  12);

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
