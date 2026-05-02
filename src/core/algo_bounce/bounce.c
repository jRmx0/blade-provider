#include "bounce.h"
#include "internal.h"

#include <string.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "check/bounce_check.h"
#include "metadata/bounce_metadata.c"
#include "../common/headland.h"

static char *bounce_create_error_json(const char *code, const char *message)
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

char *bounce_get_metadata_json(void)
{
    return bounce_build_metadata_json();
}

char *bounce_run_compute(const char *input_environment_json)
{
    input_environment_t environment;
    bounce_check_result_t check_result;

    if (!bounce_check_request_json(input_environment_json, &environment, &check_result))
    {
        return bounce_create_error_json(check_result.code, check_result.message);
    }

    // Stage 3: Compute headland if enabled
    headland_t headland;
    if (environment.headland)
    {
        int hl_result = compute_headland(&environment, &headland);
        if (hl_result != 0)
        {
            bounce_free_input_environment(&environment);
            if (hl_result == -2)
                return bounce_create_error_json("headland_allocation_failed", "Failed to allocate headland computation.");
            if (hl_result == -10)
                return bounce_create_error_json("headland_overlap", "Two or more expanded obstacles overlap.");
            if (hl_result == -11)
                return bounce_create_error_json("headland_escape", "An expanded obstacle escapes the shrunken zone.");
            return bounce_create_error_json("headland_error", "Headland computation failed.");
        }
    }
    else
    {
        // Initialize empty headland structure for consistency
        memset(&headland, 0, sizeof(headland_t));
    }

    // Build response
    cJSON *result = cJSON_CreateObject();
    if (result == NULL)
    {
        if (environment.headland)
            free_headland(&headland);
        bounce_free_input_environment(&environment);
        return bounce_create_error_json("allocation_failed", "Failed to allocate result object.");
    }

    cJSON *coverage_path_plan = cJSON_CreateObject();
    cJSON *segments = cJSON_CreateArray();

    if (coverage_path_plan == NULL || segments == NULL)
    {
        cJSON_Delete(result);
        cJSON_Delete(coverage_path_plan);
        cJSON_Delete(segments);
        if (environment.headland)
            free_headland(&headland);
        bounce_free_input_environment(&environment);
        return bounce_create_error_json("allocation_failed", "Failed to allocate result components.");
    }

    cJSON_AddItemToObject(coverage_path_plan, "segments", segments);
    cJSON_AddItemToObject(result, "coveragePathPlan", coverage_path_plan);

    cJSON *debug = cJSON_CreateObject();
    cJSON *debug_layers = cJSON_CreateArray();
    if (debug == NULL || debug_layers == NULL)
    {
        cJSON_Delete(result);
        if (environment.headland)
            free_headland(&headland);
        bounce_free_input_environment(&environment);
        return bounce_create_error_json("allocation_failed", "Failed to allocate debug layers.");
    }

    cJSON_AddItemToObject(debug, "layers", debug_layers);
    cJSON_AddItemToObject(result, "debug", debug);

    // --- Populate debug layers ---

    // Coverage layer (empty for now - Stage 4 will populate)
    {
        cJSON *layer = cJSON_CreateObject();
        if (layer != NULL)
        {
            cJSON_AddStringToObject(layer, "name", "Coverage");
            cJSON *segments_arr = cJSON_CreateArray();
            if (segments_arr != NULL)
                cJSON_AddItemToObject(layer, "segments", segments_arr);
            cJSON_AddItemToArray(debug_layers, layer);
        }
    }

    // Expanded Obstacles layer (from headland computation)
    if (environment.headland && headland.expanded_obstacles != NULL && headland.expanded_obstacle_count > 0)
    {
        cJSON *layer = cJSON_CreateObject();
        if (layer != NULL)
        {
            cJSON_AddStringToObject(layer, "name", "Expanded Obstacles");
            cJSON *polygons_arr = cJSON_CreateArray();
            if (polygons_arr != NULL)
            {
                for (uint32_t i = 0; i < headland.expanded_obstacle_count; i++)
                {
                    cJSON *poly = cJSON_CreateObject();
                    if (poly != NULL)
                    {
                        cJSON *vertices = cJSON_CreateArray();
                        if (vertices != NULL)
                        {
                            for (uint32_t j = 0; j < headland.expanded_obstacles[i].vertex_count; j++)
                            {
                                cJSON *point = cJSON_CreateObject();
                                if (point != NULL)
                                {
                                    cJSON_AddNumberToObject(point, "x", headland.expanded_obstacles[i].vertices[j].x);
                                    cJSON_AddNumberToObject(point, "y", headland.expanded_obstacles[i].vertices[j].y);
                                    cJSON_AddItemToArray(vertices, point);
                                }
                            }
                            cJSON_AddItemToObject(poly, "vertices", vertices);
                        }
                        cJSON_AddItemToArray(polygons_arr, poly);
                    }
                }
                cJSON_AddItemToObject(layer, "polygons", polygons_arr);
            }
            cJSON_AddItemToArray(debug_layers, layer);
        }
    }

    // Shrunken Zones layer (from headland computation)
    if (environment.headland && headland.shrunken_zone.vertices != NULL && headland.shrunken_zone.vertex_count > 0)
    {
        cJSON *layer = cJSON_CreateObject();
        if (layer != NULL)
        {
            cJSON_AddStringToObject(layer, "name", "Shrunken Zones");
            cJSON *polygons_arr = cJSON_CreateArray();
            if (polygons_arr != NULL)
            {
                cJSON *poly = cJSON_CreateObject();
                if (poly != NULL)
                {
                    cJSON *vertices = cJSON_CreateArray();
                    if (vertices != NULL)
                    {
                        for (uint32_t j = 0; j < headland.shrunken_zone.vertex_count; j++)
                        {
                            cJSON *point = cJSON_CreateObject();
                            if (point != NULL)
                            {
                                cJSON_AddNumberToObject(point, "x", headland.shrunken_zone.vertices[j].x);
                                cJSON_AddNumberToObject(point, "y", headland.shrunken_zone.vertices[j].y);
                                cJSON_AddItemToArray(vertices, point);
                            }
                        }
                        cJSON_AddItemToObject(poly, "vertices", vertices);
                    }
                    cJSON_AddItemToArray(polygons_arr, poly);
                }
                cJSON_AddItemToObject(layer, "polygons", polygons_arr);
            }
            cJSON_AddItemToArray(debug_layers, layer);
        }
    }

    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);

    // Cleanup
    if (environment.headland)
        free_headland(&headland);
    bounce_free_input_environment(&environment);

    return json;
}

char *bounce_compute(const char *input_environment_json)
{
    return bounce_run_compute(input_environment_json);
}