#include "bounce.h"
#include "internal.h"

#include <string.h>
#include "../../../dependencies/cJSON/cJSON.h"
#include "../metadata/metadata_json.h"
#include "../metadata/metadata_types.h"
#include "check/bounce_check.h"
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
    cJSON *algorithm = cJSON_CreateObject();
    if (algorithm == NULL)
    {
        return NULL;
    }

    cJSON_AddNumberToObject(algorithm, "id", 2);
    cJSON_AddStringToObject(algorithm, "name", "Bounce");

    cJSON *parameters = cJSON_CreateArray();
    cJSON *layers = cJSON_CreateArray();
    cJSON *metrics = cJSON_CreateArray();
    if (parameters == NULL || layers == NULL || metrics == NULL)
    {
        cJSON_Delete(parameters);
        cJSON_Delete(layers);
        cJSON_Delete(metrics);
        cJSON_Delete(algorithm);
        return NULL;
    }

    cJSON_AddItemToObject(algorithm, "parameters", parameters);
    cJSON_AddItemToObject(algorithm, "layers", layers);
    cJSON_AddItemToObject(algorithm, "metrics", metrics);

    // Stage 2 Parameters - Only Path Width and Headland
    metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "15", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
    metadata_add_parameter(parameters, 2, "Headland", BCD_METADATA_PARAM_TYPE_BOOLEAN, NULL, 0, "true", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0);

    // Stage 2 Layers
    // Coverage layer
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 1);
        cJSON_AddStringToObject(layer, "computeLayer", "coveragePathPlan.coverage");
        cJSON_AddStringToObject(layer, "name", "Coverage");
        cJSON_AddStringToObject(layer, "layerType", "Line");

        cJSON *general = NULL, *point = NULL, *line = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, &line, NULL, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "true");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "100");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT, METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR, METADATA_STYLE_TYPE_COLOR, "#3b82f6");
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH, METADATA_STYLE_TYPE_PIXELS, "2");
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "solid");
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_START, METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM, NULL);
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_END, METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM, NULL);
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID, METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM, NULL);
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    // Expanded Obstacles layer
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 2);
        cJSON_AddStringToObject(layer, "computeLayer", "headlandExpandedObstacleBorders");
        cJSON_AddStringToObject(layer, "name", "Expanded Obstacles");
        cJSON_AddStringToObject(layer, "layerType", "Polygon");

        cJSON *general = NULL, *point = NULL, *polygon = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, &polygon, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "85");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR, METADATA_STYLE_TYPE_COLOR, "#ef4444");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH, METADATA_STYLE_TYPE_PIXELS, "2");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "dotted");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE, METADATA_STYLE_TYPE_FILL_STYLE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE, METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    // Shrunken Zones layer
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 3);
        cJSON_AddStringToObject(layer, "computeLayer", "headlandShrunkenZoneBorder");
        cJSON_AddStringToObject(layer, "name", "Shrunken Zones");
        cJSON_AddStringToObject(layer, "layerType", "Polygon");

        cJSON *general = NULL, *point = NULL, *polygon = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, &polygon, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "80");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR, METADATA_STYLE_TYPE_COLOR, "#22c55e");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH, METADATA_STYLE_TYPE_PIXELS, "2");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "dotted");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE, METADATA_STYLE_TYPE_FILL_STYLE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE, METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
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