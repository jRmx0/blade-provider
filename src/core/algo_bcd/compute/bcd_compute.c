/**
 * bcd_compute.c
 *
 * Validates and then orchestrates the BCD computation pipeline.
 * The request JSON is owned by the API/dispatcher boundary,
 * while BCD-specific request checking lives in check/bcd_check.c.
 * After validation succeeds, the current blade-provider BCD path
 * still returns the native not_implemented placeholder.
 *
 * Adding a new step:
 *   1. Implement the step file in compute/
 *   2. Call it in sequence here and report progress
 *
 * Dependencies: internal.h, step files
 */

#include "../internal.h"
#include "../check/bcd_check.h"
#include "../../../../dependencies/cJSON/cjson.h"

static char *bcd_create_error_json(const char *code, const char *message)
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

char *bcd_run_compute(const char *input_environment_json)
{
	input_environment_t environment;
	bcd_check_result_t check_result;
	if (!bcd_check_request_json(input_environment_json, &environment, &check_result))
	{
		return bcd_create_error_json(check_result.code, check_result.message);
	}

	char *result = coverage_path_planning_process(&environment);
	free_input_environment(&environment);
	return result;
}