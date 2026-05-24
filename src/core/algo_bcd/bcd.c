/**
 * bcd.c
 *
 * Main coordinator for the BCD algorithm module.
 * Implements the public interface declared in bcd.h.
 * Delegates metadata requests to metadata/bcd_metadata.c and
 * computation requests to compute/bcd_compute.c.
 * Owns all JSON orchestration: validation, error serialization, result
 * serialization, and performance metric attachment.
 * Contains no business logic of its own.
 *
 * Dependencies: metadata/bcd_metadata.c, compute/bcd_compute.c
 */

#include "bcd.h"
#include "parser/bcd_parser.h"
#include "../../../dependencies/cJSON/cJSON.h"
#include "../../../dependencies/allocator/allocator.h"
#include "../../../dependencies/timer/timer.h"
#include <stdlib.h>
#include <string.h>

#define BCD_BYTES_PER_KB 1024.0

#include "parser/bcd_parser.c"
#include "metadata/bcd_metadata.c"
#include "compute/bcd_compute.c"

// Entrypoint error serialization

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

char *bcd_get_metadata_json(void)
{
	return bcd_build_metadata_json();
}

char *bcd_compute(const char *input_environment_json)
{
	input_environment_t environment;
	bcd_check_result_t check_result;

	if (!bcd_parse_request_json(input_environment_json, &environment, &check_result))
	{
		return bcd_create_error_json(check_result.code, check_result.message);
	}

	bool track_memory_usage = environment.track_memory_usage;
	bool track_processing_time = environment.track_processing_time;

	if (track_memory_usage)
	{
		va_free_tracking_data();
		va_free_stage_markers();
		va_tracking_enable();
		va_tracking_set_baseline();
	}
	else
	{
		va_tracking_disable();
	}

	if (track_processing_time)
	{
		tm_reset();
		tm_set_unit(TM_UNIT_MS);
		tm_tracking_enable();
		tm_start();
	}
	else
	{
		tm_tracking_disable();
	}

	bcd_result_t *result = bcd_run_compute(&environment);

	long *samples = NULL;
	size_t sample_count = 0;
	long baseline = 0;
	const va_stage_marker_t *markers = NULL;
	size_t marker_count = 0;

	if (track_memory_usage)
	{
		samples = va_get_tracking_data();
		sample_count = va_get_tracking_count();
		baseline = va_get_baseline();
		markers = va_get_stage_markers();
		marker_count = va_get_stage_marker_count();
		va_tracking_disable();
	}

	double total_time_ms = 0.0;
	const tm_stage_marker_t *time_markers = NULL;
	size_t time_marker_count = 0;

	if (track_processing_time)
	{
		total_time_ms = tm_stop();
		time_markers = tm_get_stage_markers();
		time_marker_count = tm_get_stage_marker_count();
		tm_tracking_disable();
	}

	free_input_environment(&environment);

	if (result == NULL)
	{
		va_free_tracking_data();
		va_free_stage_markers();
		tm_free_tracking_data();
		tm_free_stage_markers();
		return bcd_create_error_json("BCD_COMPUTE_FAILED", "BCD computation failed.");
	}

	cJSON *root = bcd_build_result_json_tree(result);
	bcd_result_free(result);

	if (root == NULL)
	{
		va_free_tracking_data();
		va_free_stage_markers();
		tm_free_tracking_data();
		tm_free_stage_markers();
		return bcd_create_error_json("serialization_failed", "BCD failed to build result JSON tree.");
	}

	if ((track_memory_usage && samples != NULL && sample_count > 0) ||
		(track_processing_time && time_marker_count > 0))
	{
		cJSON *performance = cJSON_CreateObject();
		if (performance != NULL)
		{
			cJSON *metrics_arr = cJSON_CreateArray();
			cJSON_AddItemToObject(performance, "metrics", metrics_arr);

			if (track_memory_usage && samples != NULL && sample_count > 0)
			{
				/* id=1 — time-series of working-set snapshots in KB, with stage markers */
				cJSON *metric1 = cJSON_CreateObject();
				cJSON_AddNumberToObject(metric1, "id", 1);
				cJSON *val_arr = cJSON_CreateArray();
				for (size_t i = 0; i < sample_count; ++i)
				{
					double kb = (double)samples[i] / BCD_BYTES_PER_KB;
					cJSON_AddItemToArray(val_arr, cJSON_CreateNumber(kb));
				}
				cJSON_AddItemToObject(metric1, "value", val_arr);

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
					cJSON_AddItemToObject(metric1, "stages", stages_arr);
				}
				cJSON_AddItemToArray(metrics_arr, metric1);

				/* id=2 — baseline working-set floor in KB */
				cJSON *metric2 = cJSON_CreateObject();
				cJSON_AddNumberToObject(metric2, "id", 2);
				cJSON_AddNumberToObject(metric2, "value", (double)baseline / BCD_BYTES_PER_KB);
				cJSON_AddItemToArray(metrics_arr, metric2);

				/* id=3 — peak working-set delta in KB */
				long bcd_peak = 0;
				for (size_t i = 0; i < sample_count; ++i)
					if (samples[i] > bcd_peak)
						bcd_peak = samples[i];
				cJSON *metric3 = cJSON_CreateObject();
				cJSON_AddNumberToObject(metric3, "id", 3);
				cJSON_AddNumberToObject(metric3, "value", (double)bcd_peak / BCD_BYTES_PER_KB);
				cJSON_AddItemToArray(metrics_arr, metric3);
			}

			if (track_processing_time && time_marker_count > 0)
			{
				/* id=4 — total processing time in ms */
				cJSON *metric4 = cJSON_CreateObject();
				cJSON_AddNumberToObject(metric4, "id", 4);
				cJSON_AddNumberToObject(metric4, "value", total_time_ms);
				cJSON_AddItemToArray(metrics_arr, metric4);

				/* id=5 — per-section durations (bar chart) */
				cJSON *metric5 = cJSON_CreateObject();
				cJSON_AddNumberToObject(metric5, "id", 5);
				cJSON *sections_arr = cJSON_CreateArray();
				for (size_t i = 0; i < time_marker_count; ++i)
				{
					cJSON *jsec = cJSON_CreateObject();
					cJSON_AddStringToObject(jsec, "label", time_markers[i].label);
					cJSON_AddNumberToObject(jsec, "duration", time_markers[i].duration);
					cJSON_AddItemToArray(sections_arr, jsec);
				}
				cJSON_AddItemToObject(metric5, "value", sections_arr);
				cJSON_AddItemToArray(metrics_arr, metric5);
			}

			cJSON_AddItemToObject(root, "performance", performance);
		}
	}

	va_free_tracking_data();
	va_free_stage_markers();
	tm_free_tracking_data();
	tm_free_stage_markers();

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json;
}
