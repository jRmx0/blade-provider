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
#include "../../../../dependencies/cJSON/cJSON.h"
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

	/* Unhook cJSON before the compute pipeline so serialize_result_json
	 * builds the result tree with CRT allocations. Only BCD algorithm
	 * data (cvectors, polygon arrays, check parse) appears in the
	 * working-set arc; serialization overhead is excluded. */
	if (track_memory_usage)
		cJSON_InitHooks(NULL);

	cJSON *root = coverage_path_planning_process(&environment);
	free_input_environment(&environment);

	if (track_memory_usage)
	{
		/* All compute data and environment polygons have now been freed via
		 * va_free. Snapshot captures the full arc including the drop. */
		long   *samples  = va_get_tracking_data();
		size_t  count    = va_get_tracking_count();
		long    baseline = va_get_baseline();
		va_tracking_disable();

		/* Only attach performance to successful results. */
		if (root != NULL && cJSON_GetObjectItemCaseSensitive(root, "coveragePathPlan") != NULL)
		{
			cJSON *perf = cJSON_CreateObject();
			cJSON *metrics_arr = cJSON_CreateArray();
			cJSON_AddItemToObject(perf, "metrics", metrics_arr);
			cJSON *metric = cJSON_CreateObject();
			cJSON_AddNumberToObject(metric, "id", 1);
			cJSON *val_arr = cJSON_CreateArray();
			for (size_t i = 0; i < count; ++i)
				cJSON_AddItemToArray(val_arr, cJSON_CreateNumber((double)samples[i]));
			cJSON_AddItemToObject(metric, "value", val_arr);
			cJSON_AddItemToArray(metrics_arr, metric);

			/* id=2 Baseline Memory Usage — absolute working-set floor in bytes
			 * before the BCD tracking window began. Single-value metric. */
			cJSON *baseline_metric = cJSON_CreateObject();
			cJSON_AddNumberToObject(baseline_metric, "id", 2);
			cJSON_AddNumberToObject(baseline_metric, "value", (double)baseline);
			cJSON_AddItemToArray(metrics_arr, baseline_metric);

			cJSON_AddItemToObject(root, "performance", perf);
		}
		va_free_tracking_data();
	}

	va_tracking_enable();

	/* cJSON is no longer hooked; result tree and output string are
	 * CRT-backed and safe for the caller to free() directly. */
	char *out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return out;
}