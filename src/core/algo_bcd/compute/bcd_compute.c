/**
 * bcd_compute.c
 *
 * Validates and then orchestrates the BCD computation pipeline.
 * The request JSON is owned by the API/dispatcher boundary,
 * while BCD-specific request checking lives in check/bcd_check.c.
 * After validation succeeds, delegates to compute_runner/bcd_runner.c.
 *
 * Dependencies: internal.h, bcd_runner.c, bcd_core/ sources
 */

#include "../internal.h"
#include "../check/bcd_check.h"
#include "../../../../dependencies/cJSON/cjson.h"

#include "compute_runner/bcd_core/bcd_event_list_building.c"
#include "compute_runner/bcd_core/bcd_cell_computation.c"
#include "compute_runner/bcd_core/bcd_coverage_planning.c"
#include "compute_runner/bcd_core/bcd_motion_planning.c"
#include "compute_runner/bcd_runner.c"

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