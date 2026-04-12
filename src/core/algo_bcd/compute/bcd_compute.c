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
#include "../../../../dependencies/allocator/allocator.h"
#include <stdlib.h>
#include <string.h>

/* Windows SDK defines IN/OUT as SAL annotation macros which collide with
 * the bcd_event_type_t enum values of the same name. */
#undef IN
#undef OUT

#include "compute_runner/bcd_core/bcd_event_list_building.c"
#include "compute_runner/bcd_core/bcd_cell_computation.c"
#include "compute_runner/bcd_core/bcd_coverage_planning.c"
#include "compute_runner/bcd_core/bcd_motion_planning.c"
#include "compute_runner/bcd_core/bcd_geometry.c"
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

/* Pre-extract the "Track Memory Usage" boolean from the request before any
 * allocator or cJSON hook is configured. Uses the default CRT-backed cJSON
 * so this infrastructure parse is never included in the tracked samples.
 * Defaults to false on any parse failure. */
static bool bcd_prefetch_track_flag(const char *json)
{
	if (json == NULL)
	{
		return false;
	}

	cJSON *root = cJSON_Parse(json);
	if (!cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		return false;
	}

	const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
	if (!cJSON_IsObject(parameters))
	{
		cJSON_Delete(root);
		return false;
	}

	const cJSON *value = cJSON_GetObjectItemCaseSensitive(parameters, "Track Memory Usage");
	bool result = false;
	if (cJSON_IsBool(value))
	{
		result = cJSON_IsTrue(value);
	}
	else if (cJSON_IsString(value) && value->valuestring != NULL)
	{
		result = strcmp(value->valuestring, "true") == 0;
	}

	cJSON_Delete(root);
	return result;
}

char *bcd_run_compute(const char *input_environment_json)
{
	bool track_memory_usage = bcd_prefetch_track_flag(input_environment_json);

	if (track_memory_usage)
	{
		/* Hook cJSON so its internal allocations go through VirtualAlloc and
		 * are captured alongside the cvector tracking samples. */
		cJSON_Hooks hooks = { va_malloc, va_free };
		cJSON_InitHooks(&hooks);

		va_free_tracking_data();
		va_tracking_enable();
		va_tracking_set_baseline();
	}
	else
	{
		va_tracking_disable();
		va_free_tracking_data();
	}

	input_environment_t environment;
	bcd_check_result_t check_result;
	if (!bcd_check_request_json(input_environment_json, &environment, &check_result))
	{
		va_tracking_enable();
		if (track_memory_usage) cJSON_InitHooks(NULL);
		return bcd_create_error_json(check_result.code, check_result.message);
	}

	char *va_result = coverage_path_planning_process(&environment);
	free_input_environment(&environment);

	va_tracking_enable();

	if (track_memory_usage)
	{
		cJSON_InitHooks(NULL);

		/* coverage_path_planning_process serialised its output while cJSON was
		 * hooked to va_malloc, so the result string is VirtualAlloc-backed.
		 * Copy it to the CRT heap so the caller can safely pass it to free(). */
		char *result = NULL;
		if (va_result != NULL)
		{
			size_t len = strlen(va_result) + 1;
			result = (char *)malloc(len);
			if (result != NULL)
				memcpy(result, va_result, len);
			va_free(va_result);
		}
		return result;
	}

	return va_result;
}