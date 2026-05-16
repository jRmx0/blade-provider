#include "../../metadata/metadata_json.h"
#include "../../metadata/metadata_types.h"

char *cstar_build_metadata_json(void)
{
    cJSON *algorithm = cJSON_CreateObject();
    if (algorithm == NULL)
    {
        return NULL;
    }

    cJSON_AddNumberToObject(algorithm, "id", 3);
    cJSON_AddStringToObject(algorithm, "name", "C Star");

    cJSON *parameters = cJSON_CreateArray();
    if (parameters == NULL)
    {
        cJSON_Delete(algorithm);
        return NULL;
    }

    cJSON_AddItemToObject(algorithm, "parameters", parameters);

    metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "20", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 2, "Sensor Range", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "300", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0, 0, 0.0, NULL);
    metadata_add_parameter(parameters, 3, "Frontier Spacing Multiplier", BCD_METADATA_PARAM_TYPE_INTEGER, NULL, 0, "1", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 1.0, 0, 0.0, NULL);

    /* ── Layers ─────────────────────────────────────────────────────────── */

    cJSON *layers = cJSON_CreateArray();
    if (layers == NULL)
    {
        cJSON_Delete(algorithm);
        return NULL;
    }

    cJSON_AddItemToObject(algorithm, "layers", layers);

    /* Helper macro for the repeated line-layer boilerplate */
#define CSTAR_LINE_LAYER(layer_id, compute_layer_str, layer_name_str, z_index_str, edge_color_str, edge_style_str, visible_str)                                  \
    {                                                                                                                                                            \
        cJSON *layer = cJSON_CreateObject();                                                                                                                     \
        cJSON_AddNumberToObject(layer, "id", (layer_id));                                                                                                        \
        cJSON_AddStringToObject(layer, "computeLayer", (compute_layer_str));                                                                                     \
        cJSON_AddStringToObject(layer, "name", (layer_name_str));                                                                                                \
        cJSON_AddStringToObject(layer, "layerType", "Line");                                                                                                     \
        cJSON *general = NULL, *point = NULL, *line = NULL;                                                                                                      \
        cJSON *style = metadata_create_style_object(&general, &point, &line, NULL, NULL);                                                                        \
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, (visible_str));                                           \
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, (z_index_str));                                           \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);                                         \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, NULL);                                                  \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);                                         \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT, METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);                             \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);                                             \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, NULL);                                            \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);                                 \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);                                               \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);                                                 \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);                                            \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);                                \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);                                    \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);                                               \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);                                              \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);                                         \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);                             \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);                                 \
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);                                            \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR, METADATA_STYLE_TYPE_COLOR, (edge_color_str));                                     \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH, METADATA_STYLE_TYPE_PIXELS, "1");                                                 \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, (edge_style_str));                         \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_START, METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM, NULL);                                \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_END, METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM, NULL);                                    \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID, METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM, METADATA_LINE_ARROW_MID_END_ARROW_NOTCH); \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING, METADATA_STYLE_TYPE_PIXELS, "25");                                         \
        metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE, METADATA_STYLE_TYPE_PIXELS, "2");                                                 \
        cJSON_AddItemToObject(layer, "style", style);                                                                                                            \
        cJSON_AddItemToArray(layers, layer);                                                                                                                     \
    }

    /* ── coveragePathPlan layers ─────────────────────────────────────────── */

    /* id=1  Coverage — solid main sweep path, highest z-index */
    CSTAR_LINE_LAYER(1, "coveragePathPlan.coverage", "Coverage", "100", "#60a5fa", "solid", "true")

    /* id=2  Coverage Transit — start→first node, last node→end */
    CSTAR_LINE_LAYER(2, "coveragePathPlan.coverageTransit", "Coverage Transit", "92", "#60a5fa", "dashed", "true")

    /* id=3  Retreat Transit — dead-end A* escape path */
    CSTAR_LINE_LAYER(3, "coveragePathPlan.retreatTransit", "Retreat Transit", "88", "#f87171", "dashed", "true")

    /* id=4  Hole Coverage — TSP loop inside a detected coverage hole */
    CSTAR_LINE_LAYER(4, "coveragePathPlan.holeCoverage", "Hole Coverage", "96", "#a78bfa", "solid", "true")

    /* id=5  Hole Transit — move to/from hole subproblem entry/exit node */
    CSTAR_LINE_LAYER(5, "coveragePathPlan.holeTransit", "Hole Transit", "90", "#a78bfa", "dashed", "true")

    /* ── Debug layers ────────────────────────────────────────────────────── */

    /* id=10  RCG nodes (Point) */
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 10);
        cJSON_AddStringToObject(layer, "computeLayer", "rcgNodeList");
        cJSON_AddStringToObject(layer, "name", "RCG Nodes");
        cJSON_AddStringToObject(layer, "layerType", "Point");

        cJSON *general = NULL, *point = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, NULL, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "110");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, "3");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT, METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, "#94a3b8");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, "1");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "solid");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, "#1e293b");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR, METADATA_STYLE_TYPE_COLOR, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR, METADATA_STYLE_TYPE_COLOR, "#94a3b8");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE, METADATA_STYLE_TYPE_PIXELS, "10");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT, METADATA_STYLE_TYPE_PLACEMENT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET, METADATA_STYLE_TYPE_PIXELS, NULL);

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    /* id=11  RCG edges (Line) */
    CSTAR_LINE_LAYER(11, "rcgEdgeList", "RCG Edges", "108", "#334155", "solid", "false")

    /* id=12  Frontier samples (Point) */
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 12);
        cJSON_AddStringToObject(layer, "computeLayer", "frontierSampleList");
        cJSON_AddStringToObject(layer, "name", "Frontier Samples");
        cJSON_AddStringToObject(layer, "layerType", "Point");

        cJSON *general = NULL, *point = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, NULL, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "112");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, "3");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT, METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, "#86efac");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, "1");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "solid");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, "#166534");
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

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    /* id=13  Retreat nodes (Point) */
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 13);
        cJSON_AddStringToObject(layer, "computeLayer", "retreatNodeList");
        cJSON_AddStringToObject(layer, "name", "Retreat Nodes");
        cJSON_AddStringToObject(layer, "layerType", "Point");

        cJSON *general = NULL, *point = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, NULL, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "114");

        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE, METADATA_STYLE_TYPE_POINT_SHAPE_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS, METADATA_STYLE_TYPE_PIXELS, "4");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING, METADATA_STYLE_TYPE_PIXELS, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT, METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR, "#fca5a5");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS, "1");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "solid");
        metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, "#7f1d1d");
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

        cJSON_AddItemToObject(layer, "style", style);
        cJSON_AddItemToArray(layers, layer);
    }

    /* id=14  Coverage holes (Polygon) */
    {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddNumberToObject(layer, "id", 14);
        cJSON_AddStringToObject(layer, "computeLayer", "coverageHoleList");
        cJSON_AddStringToObject(layer, "name", "Coverage Holes");
        cJSON_AddStringToObject(layer, "layerType", "Polygon");

        cJSON *general = NULL, *point = NULL, *polygon = NULL;
        cJSON *style = metadata_create_style_object(&general, &point, NULL, &polygon, NULL);

        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE, METADATA_STYLE_TYPE_BOOLEAN, "false");
        metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX, METADATA_STYLE_TYPE_INTEGER, "116");

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

        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR, METADATA_STYLE_TYPE_COLOR, "#fbbf24");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH, METADATA_STYLE_TYPE_PIXELS, "1");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, "dashed");
        metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR, METADATA_STYLE_TYPE_COLOR, "#78350f");
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

#undef CSTAR_LINE_LAYER

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
}