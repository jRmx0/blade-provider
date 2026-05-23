#include "cstar_serialize.h"
#include "../../common/debug_serialize.h"
#include "../../../../dependencies/cJSON/cJSON.h"
#include <stdlib.h>
#include <string.h>

static cJSON *cstar_serialize_debug_point_entry_json(const cstar_debug_point_entry_t *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }

    cJSON *json_entry = cJSON_CreateObject();
    cJSON *json_point = cJSON_CreateObject();
    if (json_entry == NULL || json_point == NULL)
    {
        cJSON_Delete(json_entry);
        cJSON_Delete(json_point);
        return NULL;
    }

    cJSON_AddNumberToObject(json_entry, "id", entry->id);
    cJSON_AddNumberToObject(json_point, "x", entry->point.x);
    cJSON_AddNumberToObject(json_point, "y", entry->point.y);
    cJSON_AddItemToObject(json_entry, "point", json_point);
    return json_entry;
}

static cJSON *cstar_serialize_debug_segment_entry_json(const cstar_debug_segment_entry_t *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }

    cJSON *json_entry = cJSON_CreateObject();
    cJSON *json_path = cJSON_CreateArray();
    if (json_entry == NULL || json_path == NULL)
    {
        cJSON_Delete(json_entry);
        cJSON_Delete(json_path);
        return NULL;
    }

    cJSON_AddNumberToObject(json_entry, "id", entry->id);
    cJSON_AddStringToObject(json_entry, "type", entry->type != NULL ? entry->type : "segment");
    cJSON_AddItemToObject(json_entry, "path", json_path);

    for (int i = 0; i < entry->path_count; ++i)
    {
        cJSON *json_waypoint = cJSON_CreateObject();
        cJSON *json_point = cJSON_CreateObject();
        if (json_waypoint == NULL || json_point == NULL)
        {
            cJSON_Delete(json_waypoint);
            cJSON_Delete(json_point);
            continue;
        }

        cJSON_AddNumberToObject(json_waypoint, "id", i);
        cJSON_AddNumberToObject(json_point, "x", entry->path[i].x);
        cJSON_AddNumberToObject(json_point, "y", entry->path[i].y);
        cJSON_AddItemToObject(json_waypoint, "point", json_point);
        cJSON_AddItemToArray(json_path, json_waypoint);
    }

    return json_entry;
}

static cJSON *cstar_serialize_debug_polygon_entry_json(const cstar_debug_polygon_entry_t *entry)
{
    if (entry == NULL)
    {
        return NULL;
    }

    cJSON *json_entry = cJSON_CreateObject();
    cJSON *json_vertices = cJSON_CreateArray();
    if (json_entry == NULL || json_vertices == NULL)
    {
        cJSON_Delete(json_entry);
        cJSON_Delete(json_vertices);
        return NULL;
    }

    cJSON_AddNumberToObject(json_entry, "id", entry->id);
    cJSON_AddItemToObject(json_entry, "vertices", json_vertices);

    for (int i = 0; i < entry->vertex_count; ++i)
    {
        cJSON *json_vertex = cJSON_CreateObject();
        if (json_vertex == NULL)
        {
            continue;
        }

        cJSON_AddNumberToObject(json_vertex, "x", entry->vertices[i].x);
        cJSON_AddNumberToObject(json_vertex, "y", entry->vertices[i].y);
        cJSON_AddItemToArray(json_vertices, json_vertex);
    }

    return json_entry;
}

static cJSON *cstar_serialize_debug_layers_json(const cstar_debug_layers_t *debug_layers)
{
    cJSON *json_layers = cJSON_CreateArray();
    if (json_layers == NULL)
    {
        return NULL;
    }

    if (debug_layers == NULL || debug_layers->layers == NULL)
    {
        return json_layers;
    }

    for (int i = 0; i < debug_layers->layer_count; ++i)
    {
        const cstar_debug_layer_t *layer = &debug_layers->layers[i];
        cJSON *json_layer = cJSON_CreateObject();
        cJSON *json_list = cJSON_CreateArray();
        if (json_layer == NULL || json_list == NULL)
        {
            cJSON_Delete(json_layer);
            cJSON_Delete(json_list);
            cJSON_Delete(json_layers);
            return NULL;
        }

        cJSON_AddNumberToObject(json_layer, "id", layer->id);
        cJSON_AddStringToObject(json_layer, "source", layer->source != NULL ? layer->source : "unknown");
        cJSON_AddItemToObject(json_layer, "list", json_list);

        if (layer->list_type == CSTAR_DEBUG_LIST_POINTS)
        {
            for (int j = 0; j < layer->list.points.count; ++j)
            {
                cJSON *json_entry = cstar_serialize_debug_point_entry_json(&layer->list.points.items[j]);
                if (json_entry != NULL)
                {
                    cJSON_AddItemToArray(json_list, json_entry);
                }
            }
        }
        else if (layer->list_type == CSTAR_DEBUG_LIST_SEGMENTS)
        {
            for (int j = 0; j < layer->list.segments.count; ++j)
            {
                cJSON *json_entry = cstar_serialize_debug_segment_entry_json(&layer->list.segments.items[j]);
                if (json_entry != NULL)
                {
                    cJSON_AddItemToArray(json_list, json_entry);
                }
            }
        }
        else if (layer->list_type == CSTAR_DEBUG_LIST_POLYGONS)
        {
            for (int j = 0; j < layer->list.polygons.count; ++j)
            {
                cJSON *json_entry = cstar_serialize_debug_polygon_entry_json(&layer->list.polygons.items[j]);
                if (json_entry != NULL)
                {
                    cJSON_AddItemToArray(json_list, json_entry);
                }
            }
        }

        cJSON_AddItemToArray(json_layers, json_layer);
    }

    return json_layers;
}

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

    cJSON *root = cstar_build_result_json_tree(result);
    if (root == NULL)
    {
        return cstar_serialize_error_json("serialization_failed", "Failed to build result JSON tree.");
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json == NULL)
    {
        return cstar_serialize_error_json("serialization_failed", "Failed to convert result to JSON string.");
    }

    return json;
}

cJSON *cstar_build_result_json_tree(const cstar_coverage_path_result_t *result)
{
    if (result == NULL)
    {
        return NULL;
    }

    // Build segments array only (per API spec: CoveragePathPlan contains only segments + optional performance)
    cJSON *segments = cJSON_CreateArray();

    if (segments == NULL)
    {
        return NULL;
    }

    // Build all segments
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
            return NULL;
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
            return NULL;
        }

        // Add to main segments array (no categorization)
        cJSON_AddItemToArray(segments, json_seg);
    }

    // Build coverage plan object with only segments (per API spec)
    cJSON *coverage_path_plan = cJSON_CreateObject();
    if (coverage_path_plan == NULL)
    {
        cJSON_Delete(segments);
        return NULL;
    }

    cJSON_AddItemToObject(coverage_path_plan, "segments", segments);

    cJSON *debug = cJSON_CreateObject();
    cJSON *layers = cstar_serialize_debug_layers_json(&result->debug_layers);
    if (debug == NULL || layers == NULL)
    {
        cJSON_Delete(debug);
        cJSON_Delete(layers);
        cJSON_Delete(coverage_path_plan);
        return NULL;
    }

    cJSON_AddItemToObject(debug, "layers", layers);

    // Build root response with coveragePathPlan and debug as siblings (per API spec)
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        cJSON_Delete(coverage_path_plan);
        cJSON_Delete(debug);
        return NULL;
    }

    cJSON_AddItemToObject(root, "coveragePathPlan", coverage_path_plan);
    cJSON_AddItemToObject(root, "debug", debug);

    return root;
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

    if (result->debug_layers.layers != NULL)
    {
        for (int i = 0; i < result->debug_layers.layer_count; ++i)
        {
            cstar_debug_layer_t *layer = &result->debug_layers.layers[i];

            if (layer->list_type == CSTAR_DEBUG_LIST_SEGMENTS)
            {
                for (int j = 0; j < layer->list.segments.count; ++j)
                {
                    free(layer->list.segments.items[j].path);
                    layer->list.segments.items[j].path = NULL;
                }

                free(layer->list.segments.items);
                layer->list.segments.items = NULL;
                layer->list.segments.count = 0;
                layer->list.segments.capacity = 0;
            }
            else if (layer->list_type == CSTAR_DEBUG_LIST_POLYGONS)
            {
                for (int j = 0; j < layer->list.polygons.count; ++j)
                {
                    free(layer->list.polygons.items[j].vertices);
                    layer->list.polygons.items[j].vertices = NULL;
                }

                free(layer->list.polygons.items);
                layer->list.polygons.items = NULL;
                layer->list.polygons.count = 0;
                layer->list.polygons.capacity = 0;
            }
            else
            {
                free(layer->list.points.items);
                layer->list.points.items = NULL;
                layer->list.points.count = 0;
                layer->list.points.capacity = 0;
            }
        }

        free(result->debug_layers.layers);
        result->debug_layers.layers = NULL;
        result->debug_layers.layer_count = 0;
        result->debug_layers.layer_capacity = 0;
    }

    // Note: collection structs just reference segments from all_segments,
    // so we only free all_segments and they become invalid.
    // The collections themselves are stack-allocated in the result.

    free(result);
}
