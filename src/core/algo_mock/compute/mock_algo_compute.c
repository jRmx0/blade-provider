/**
 * mock_algo_compute.c
 *
 * Minimal compute stub for the metadata-coverage mock algorithm.
 * Validates the raw request against the mock algorithm contract first,
 * then returns a deterministic success payload for metadata/UI testing.
 */

#include "../../../../dependencies/cJSON/cjson.h"
#include "../check/mock_algo_check.h"

static char *mock_algo_create_error_json(const char *code, const char *message)
{
	cJSON *response = cJSON_CreateObject();
	if (response == NULL)
	{
		return NULL;
	}

	cJSON_AddStringToObject(response, "status", "error");
	cJSON_AddStringToObject(response, "code", code);
	cJSON_AddStringToObject(response, "message", message);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}

char *mock_algo_run_compute(const char *input_environment_json)
{
	mock_algo_check_result_t check_result;
	if (!mock_algo_check_request_json(input_environment_json, &check_result))
	{
		return mock_algo_create_error_json(check_result.code, check_result.message);
	}

	cJSON *response = cJSON_CreateObject();
	if (response == NULL)
	{
		return NULL;
	}

	cJSON_AddStringToObject(response, "status", "ok");
	cJSON_AddStringToObject(response, "algorithm", "Mock Metadata Matrix");
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
