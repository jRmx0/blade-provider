/**
 * bounce_metadata.c
 *
 * Provides static descriptive information about the Bounce algorithm.
 * Returns algorithm parameters, layers and metrics metadata used by clients.
 * Called by bounce.c in response to metadata requests from dispatcher.
 *
 * Dependencies: ../internal.h
 */

#include "../internal.h"
#include "../../metadata/metadata_json.h"
#include "../../metadata/metadata_types.h"

char *bounce_build_metadata_json(void)
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

    metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "20", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 2, "Random Bounce Offset", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 1, 100.0, "ratio");
    metadata_add_parameter(parameters, 3, "Target Coverage", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 1, 100.0, "ratio");
    metadata_add_parameter(parameters, 4, "Target Distance", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 5, "Max Iterations", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 1.0, 1, 1000000.0, "unitless");
    metadata_add_parameter(parameters, 6, "Seed", BCD_METADATA_PARAM_TYPE_STRING, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 0, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 7, "Starting Angle", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 1, 360.0, "unitless");

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

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
}
