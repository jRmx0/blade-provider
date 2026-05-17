#include "cstar_serialize.h"
#include "../../common/debug_serialize.h"
#include "../../../../dependencies/cJSON/cJSON.h"
#include <stdlib.h>
#include <string.h>

char *cstar_serialize_error_json(const char *code, const char *message)
{
    cJSON *response = cJSON_CreateObject();
    if (response == NULL)
    {
        return NULL;
    }
    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "code", code != NULL ? code : "cstar_error");
    cJSON_AddStringToObject(response, "message", message != NULL ? message : "C* compute pipeline failed.");
    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    return json;
}

char *cstar_serialize_result_json(const cstar_coverage_path_result_t *result)
{
    if (result == NULL)
    {
        return cstar_serialize_error_json("allocation_failed", "Coverage path result is NULL.");
    }

    // Build categorized arrays for quick lookup during segment addition
    cJSON *segments = cJSON_CreateArray();
    cJSON *coverage = cJSON_CreateArray();
    cJSON *coverage_transit = cJSON_CreateArray();
    cJSON *retreat_transit = cJSON_CreateArray();
    cJSON *hole_coverage = cJSON_CreateArray();
    cJSON *hole_transit = cJSON_CreateArray();

    if (segments == NULL || coverage == NULL || coverage_transit == NULL ||
        retreat_transit == NULL || hole_coverage == NULL || hole_transit == NULL)
    {
        cJSON_Delete(segments);
        cJSON_Delete(coverage);
        cJSON_Delete(coverage_transit);
        cJSON_Delete(retreat_transit);
        cJSON_Delete(hole_coverage);
        cJSON_Delete(hole_transit);
        return cstar_serialize_error_json("serialization_failed", "Failed to allocate segment arrays.");
    }

    // Build all segments and add to appropriate arrays
    for (int i = 0; i < result->segment_count; ++i)
    {
        const cstar_segment_t *seg = &result->all_segments[i];

        // Build xs and ys arrays for debug_build_segment
        float *xs = (float *)malloc(seg->path_count * sizeof(float));
        float *ys = (float *)malloc(seg->path_count * sizeof(float));

        if (xs == NULL || ys == NULL)
        {
            free(xs);
            free(ys);
            cJSON_Delete(segments);
            cJSON_Delete(coverage);
            cJSON_Delete(coverage_transit);
            cJSON_Delete(retreat_transit);
            cJSON_Delete(hole_coverage);
            cJSON_Delete(hole_transit);
            return cstar_serialize_error_json("serialization_failed", "Failed to allocate coordinate arrays.");
        }

        // Extract coordinates from path points
        for (int j = 0; j < seg->path_count; ++j)
        {
            xs[j] = seg->path[j].point.x;
            ys[j] = seg->path[j].point.y;
        }

        // Build segment JSON
        cJSON *json_seg = debug_build_segment(seg->id, seg->type, xs, ys, seg->path_count);
        free(xs);
        free(ys);

        if (json_seg == NULL)
        {
            cJSON_Delete(segments);
            cJSON_Delete(coverage);
            cJSON_Delete(coverage_transit);
            cJSON_Delete(retreat_transit);
            cJSON_Delete(hole_coverage);
            cJSON_Delete(hole_transit);
            return cstar_serialize_error_json("serialization_failed", "Failed to build segment JSON.");
        }

        // Add to main segments array
        cJSON_AddItemToArray(segments, json_seg);

        // Add copy to categorized array
        cJSON *json_seg_copy = cJSON_Duplicate(json_seg, 1);
        if (json_seg_copy == NULL)
        {
            cJSON_Delete(segments);
            cJSON_Delete(coverage);
            cJSON_Delete(coverage_transit);
            cJSON_Delete(retreat_transit);
            cJSON_Delete(hole_coverage);
            cJSON_Delete(hole_transit);
            return cstar_serialize_error_json("serialization_failed", "Failed to duplicate segment JSON.");
        }

        // Dispatch to appropriate category array
        if (strcmp(seg->type, "coverage") == 0)
        {
            cJSON_AddItemToArray(coverage, json_seg_copy);
        }
        else if (strcmp(seg->type, "coverageTransit") == 0)
        {
            cJSON_AddItemToArray(coverage_transit, json_seg_copy);
        }
        else if (strcmp(seg->type, "retreatTransit") == 0)
        {
            cJSON_AddItemToArray(retreat_transit, json_seg_copy);
        }
        else if (strcmp(seg->type, "holeCoverage") == 0)
        {
            cJSON_AddItemToArray(hole_coverage, json_seg_copy);
        }
        else if (strcmp(seg->type, "holeTransit") == 0)
        {
            cJSON_AddItemToArray(hole_transit, json_seg_copy);
        }
        else
        {
            // Unknown type, still add to segments but skip categorization
            cJSON_Delete(json_seg_copy);
        }
    }

    // Build coverage plan object
    cJSON *coverage_path_plan = cJSON_CreateObject();
    if (coverage_path_plan == NULL)
    {
        cJSON_Delete(segments);
        cJSON_Delete(coverage);
        cJSON_Delete(coverage_transit);
        cJSON_Delete(retreat_transit);
        cJSON_Delete(hole_coverage);
        cJSON_Delete(hole_transit);
        return cstar_serialize_error_json("serialization_failed", "Failed to allocate coverage plan object.");
    }

    cJSON_AddItemToObject(coverage_path_plan, "segments", segments);
    cJSON_AddItemToObject(coverage_path_plan, "coverage", coverage);
    cJSON_AddItemToObject(coverage_path_plan, "coverageTransit", coverage_transit);
    cJSON_AddItemToObject(coverage_path_plan, "retreatTransit", retreat_transit);
    cJSON_AddItemToObject(coverage_path_plan, "holeCoverage", hole_coverage);
    cJSON_AddItemToObject(coverage_path_plan, "holeTransit", hole_transit);

    // Build root response
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        cJSON_Delete(coverage_path_plan);
        return cstar_serialize_error_json("serialization_failed", "Failed to allocate root response object.");
    }

    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddItemToObject(root, "coveragePathPlan", coverage_path_plan);

    // Convert cJSON object to unformatted JSON string
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json == NULL)
    {
        return cstar_serialize_error_json("serialization_failed", "Failed to convert result to JSON string.");
    }

    return json;
}

void cstar_result_free(cstar_coverage_path_result_t *result)
{
    if (result == NULL)
    {
        return;
    }

    if (result->all_segments != NULL)
    {
        for (int i = 0; i < result->segment_count; ++i)
        {
            cstar_segment_t *seg = &result->all_segments[i];
            if (seg->type != NULL)
            {
                free(seg->type);
            }
            if (seg->path != NULL)
            {
                free(seg->path);
            }
        }
        free(result->all_segments);
    }

    if (result->debug_layers != NULL)
    {
        cJSON_Delete(result->debug_layers);
    }

    // Note: collection structs just reference segments from all_segments,
    // so we only free all_segments and they become invalid.
    // The collections themselves are stack-allocated in the result.

    free(result);
}
