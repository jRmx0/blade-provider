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
#include "compute_runner/bcd_core/bcd_pq.c"
#include "compute_runner/bcd_core/bcd_coverage_planning.c"
#include "compute_runner/bcd_core/bcd_geometry.c"
#include "compute_runner/bcd_core/bcd_pathfinding.c"
#include "compute_runner/bcd_core/bcd_funnel.c"
#include "compute_runner/bcd_core/bcd_ox_motion.c"
#include "compute_runner/bcd_core/bcd_motion_planning.c"
#include "../preprocess/bcd_preprocess.c"
#include "compute_runner/bcd_runner.c"

#define BYTES_PER_KB 1024.0

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

	/* Parse phase runs with tracking disabled. cJSON and env-struct allocations
	 * are excluded from the tracked window; only the compute pipeline is measured. */
	va_tracking_disable();
	va_free_tracking_data();

	input_environment_t environment;
	bcd_check_result_t check_result;
	if (!bcd_check_request_json(input_environment_json, &environment, &check_result))
	{
		va_tracking_enable();
		return bcd_create_error_json(check_result.code, check_result.message);
	}

	/* Baseline is set after parse with env structs already resident.
	 * Tracking window covers only the compute pipeline. */
	if (track_memory_usage)
	{
		va_free_tracking_data();
		va_free_stage_markers();
		va_tracking_enable();
		va_tracking_set_baseline();
	}

	cJSON *root = coverage_path_planning_process(&environment);
	free_input_environment(&environment);

	if (track_memory_usage)
	{
		/* All compute data and environment polygons have now been freed via
		 * va_free. Snapshot captures the full arc including the drop. */
		long *samples = va_get_tracking_data();
		size_t count = va_get_tracking_count();
		long baseline = va_get_baseline();
		const va_stage_marker_t *markers = va_get_stage_markers();
		size_t marker_count = va_get_stage_marker_count();
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
				cJSON_AddItemToArray(val_arr, cJSON_CreateNumber((double)samples[i] / BYTES_PER_KB));
			cJSON_AddItemToObject(metric, "value", val_arr);

			/* stages — nested inside metric id=1 so that each label maps
			 * directly to a sampleIndex within that metric's value array. */
			if (markers != NULL && marker_count > 0)
			{
				cJSON *stages_arr = cJSON_CreateArray();
				for (size_t i = 0; i < marker_count; ++i)
				{
					cJSON *jstage = cJSON_CreateObject();
					cJSON_AddStringToObject(jstage, "label", markers[i].label);
					cJSON_AddNumberToObject(jstage, "sampleIndex", (double)markers[i].sample_index);
					cJSON_AddItemToArray(stages_arr, jstage);
				}
				cJSON_AddItemToObject(metric, "stages", stages_arr);
			}

			cJSON_AddItemToArray(metrics_arr, metric);

			/* id=2 Baseline Memory Usage — absolute working-set floor in KB
			 * before the BCD tracking window began. Single-value metric. */
			cJSON *baseline_metric = cJSON_CreateObject();
			cJSON_AddNumberToObject(baseline_metric, "id", 2);
			cJSON_AddNumberToObject(baseline_metric, "value", (double)baseline / BYTES_PER_KB);
			cJSON_AddItemToArray(metrics_arr, baseline_metric);

			cJSON_AddItemToObject(root, "performance", perf);
		}
		va_free_tracking_data();
		va_free_stage_markers();
	}

	/* cJSON is no longer hooked; result tree and output string are
	 * CRT-backed and safe for the caller to free() directly. */
	char *out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return out;
}