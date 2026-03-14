/**
 * mock_algo_compute.c
 *
 * Minimal compute stub for the metadata-coverage mock algorithm.
 */

#include "../../../dependencies/cJSON/cjson_compat.h"

char *mock_algo_run_compute(const char *input_environment_json)
{
	cJSON *response = cJSON_CreateObject();
	if (response == NULL)
	{
		return NULL;
	}

	cJSON_AddStringToObject(response, "status", "ok");
	cJSON_AddStringToObject(response, "algorithm", "mock_metadata_matrix");
	cJSON_AddStringToObject(response, "message", "Mock metadata coverage algorithm executed.");
	cJSON_AddNumberToObject(response, "zoneCoverage", 100);
	cJSON_AddNumberToObject(response, "routeOverlap", 0);
	cJSON_AddNumberToObject(response, "turnCount", 0);
	cJSON_AddItemToObject(response, "route", cJSON_CreateArray());

	cJSON *intermediate_calculations = cJSON_CreateObject();
	if (intermediate_calculations == NULL)
	{
		cJSON_Delete(response);
		return NULL;
	}

	cJSON_AddBoolToObject(intermediate_calculations, "mock", 1);
	cJSON_AddStringToObject(intermediate_calculations, "purpose", "metadata_coverage_testing");
	cJSON_AddBoolToObject(intermediate_calculations, "inputReceived", input_environment_json != NULL);
	cJSON_AddItemToObject(response, "intermediateCalculations", intermediate_calculations);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}