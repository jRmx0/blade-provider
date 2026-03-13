/**
 * bcd_compute.c
 *
 * Orchestrates the BCD computation pipeline.
 * Receives input and a progress callback from bcd.c,
 * executes computation steps in sequence, reports progress
 * after each step, and returns the final result.
 *
 * Adding a new step:
 *   1. Implement the step file in compute/
 *   2. Call it in sequence here and report progress
 *
 * Dependencies: internal.h, step files
 */

#include "../internal.h"
#include "../../../../dependencies/cJSON/cjson_compat.h"

char *bcd_run_compute(const char *input_environment_json)
{
	cJSON *response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "status", "error");
	cJSON_AddStringToObject(response, "code", "not_implemented");
	cJSON_AddStringToObject(response, "message", "Native BCD compute has not been wired into blade-provider yet.");
	cJSON_AddBoolToObject(response, "inputReceived", input_environment_json != NULL);

	char *json = cJSON_PrintUnformatted(response);
	cJSON_Delete(response);
	return json;
}