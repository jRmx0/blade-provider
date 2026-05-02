#include "bounce.h"
#include "internal.h"

#include "../../../dependencies/cJSON/cJSON.h"
#include "../metadata/metadata_json.h"
#include "../metadata/metadata_types.h"
#include "check/bounce_check.h"

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
        cJSON_AddStringToObject(layer, "computeLayer", "expandedObstacles");
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
        cJSON_AddStringToObject(layer, "computeLayer", "shrunkZones");
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

    bounce_free_input_environment(&environment);

    // Stage 2: Return stub response with empty coverage path plan
    cJSON *result = cJSON_CreateObject();
    if (result == NULL)
    {
        return bounce_create_error_json("allocation_failed", "Failed to allocate result object.");
    }

    cJSON *coverage_path_plan = cJSON_CreateObject();
    cJSON *segments = cJSON_CreateArray();

    if (coverage_path_plan == NULL || segments == NULL)
    {
        cJSON_Delete(result);
        cJSON_Delete(coverage_path_plan);
        cJSON_Delete(segments);
        return bounce_create_error_json("allocation_failed", "Failed to allocate result components.");
    }

    cJSON_AddItemToObject(coverage_path_plan, "segments", segments);
    cJSON_AddItemToObject(result, "coveragePathPlan", coverage_path_plan);

    cJSON *debug = cJSON_CreateObject();
    cJSON *debug_layers = cJSON_CreateArray();
    if (debug == NULL || debug_layers == NULL)
    {
        cJSON_Delete(result);
        return bounce_create_error_json("allocation_failed", "Failed to allocate debug layers.");
    }

    cJSON_AddItemToObject(debug, "layers", debug_layers);
    cJSON_AddItemToObject(result, "debug", debug);

    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    return json;
}

char *bounce_compute(const char *input_environment_json)
{
    return bounce_run_compute(input_environment_json);
}