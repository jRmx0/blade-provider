
#include "cstar.h"
#include "check/cstar_check.h"
#include "parser/cstar_parser.h"
#include "serialize/cstar_serialize.h"
#include "../../../dependencies/cJSON/cJSON.h"
#include "../../../dependencies/allocator/allocator.h"
#include "../../../dependencies/timer/timer.h"
#include <stdlib.h>
#include <string.h>

#define CSTAR_BYTES_PER_KB 1024.0

#include "check/cstar_check.c"
#include "parser/cstar_parser.c"
#include "metadata/cstar_metadata.c"
#include "compute/cstar_compute.c"
#include "serialize/cstar_serialize.c"

// Entrypoint error serialization now handled by serialize module

char *cstar_get_metadata_json(void)
{
    return cstar_build_metadata_json();
}

char *cstar_compute(const char *input_environment_json)
{
    cstar_environment_t environment;
    cstar_check_result_t check_result;

    if (!cstar_validate_request_json(input_environment_json, &check_result))
    {
        return cstar_serialize_error_json(check_result.code, check_result.message);
    }

    if (!cstar_parse_request_json(input_environment_json, &environment, &check_result))
    {
        return cstar_serialize_error_json(check_result.code, check_result.message);
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

    cstar_coverage_path_result_t *result = cstar_run_compute(&environment);

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

    cstar_parser_free_environment(&environment);

    if (result == NULL)
    {
        va_free_tracking_data();
        va_free_stage_markers();
        tm_free_tracking_data();
        tm_free_stage_markers();
        return cstar_serialize_error_json("allocation_failed", "C* compute pipeline allocation failed.");
    }

    cJSON *root = cstar_build_result_json_tree(result);
    cstar_result_free(result);

    if (root == NULL)
    {
        va_free_tracking_data();
        va_free_stage_markers();
        tm_free_tracking_data();
        tm_free_stage_markers();
        return cstar_serialize_error_json("serialization_failed", "C* failed to build result JSON tree.");
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
                    double kb = (double)samples[i] / CSTAR_BYTES_PER_KB;
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
                cJSON_AddNumberToObject(metric2, "value", (double)baseline / CSTAR_BYTES_PER_KB);
                cJSON_AddItemToArray(metrics_arr, metric2);

                /* id=3 — peak working-set delta in KB */
                long cstar_peak = 0;
                for (size_t i = 0; i < sample_count; ++i)
                    if (samples[i] > cstar_peak)
                        cstar_peak = samples[i];
                cJSON *metric3 = cJSON_CreateObject();
                cJSON_AddNumberToObject(metric3, "id", 3);
                cJSON_AddNumberToObject(metric3, "value", (double)cstar_peak / CSTAR_BYTES_PER_KB);
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