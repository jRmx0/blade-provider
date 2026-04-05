/**
 * bcd_metadata.c
 *
 * Provides static descriptive information about the BCD algorithm.
 * Returns algorithm name, expected input
 * format, and produced output format.
 * Called by bcd.c in response to metadata requests from dispatcher.
 *
 * Dependencies: ../internal.h
 */

#include "../internal.h"
#include "../../metadata/metadata_json.h"

char *bcd_build_metadata_json(void)
{
	const char *format_values[] = {
		metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON)
	};
	const char *type_values[] = {
		metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE)
	};
	const char *coordsystem_values[] = {
		metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL)
	};

	cJSON *algorithm = cJSON_CreateObject();
	if (algorithm == NULL)
	{
		return NULL;
	}

	cJSON_AddNumberToObject(algorithm, "id", 1);
	cJSON_AddStringToObject(algorithm, "name", "Boustrophedon Cellular Decomposition");

	cJSON *parameters = cJSON_CreateArray();
	if (parameters == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}

	cJSON_AddItemToObject(algorithm, "parameters", parameters);

	metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "15", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 2, "Path Overlap", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "5", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE, 1, 0.0);
	metadata_add_parameter(parameters, 3, "Format", BCD_METADATA_PARAM_TYPE_ENUM, format_values, 1, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT, 0, 0.0);
	metadata_add_parameter(parameters, 4, "Type", BCD_METADATA_PARAM_TYPE_ENUM, type_values, 1, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE, 0, 0.0);
	metadata_add_parameter(parameters, 5, "Coordinate System", BCD_METADATA_PARAM_TYPE_ENUM, coordsystem_values, 1, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM, 0, 0.0);

	cJSON *layers = cJSON_CreateArray();
	if (layers == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}
	cJSON_AddItemToObject(algorithm, "layers", layers);

	/* ---- Coverage Path (Line, computeLayer = "coveragePathPlan", zIndex = 100) ---- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",           1);
		cJSON_AddStringToObject(layer, "computeLayer", "coveragePathPlan");
		cJSON_AddStringToObject(layer, "name",         "Coverage Path");
		cJSON_AddStringToObject(layer, "layerType",    "Line");

		cJSON *general = NULL, *point = NULL, *line = NULL;
		cJSON *style = metadata_create_style_object(&general, &point, &line, NULL, NULL);

		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE,               METADATA_STYLE_TYPE_BOOLEAN,            "true");
		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX,               METADATA_STYLE_TYPE_INTEGER,            "100");

		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE,             METADATA_STYLE_TYPE_POINT_SHAPE_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS,            METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING,   METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT,    METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR,      METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH,      METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE,      METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,   NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR,        METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR,          METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE,      METADATA_STYLE_TYPE_PIXELS,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT,    METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT,      METADATA_STYLE_TYPE_PLACEMENT_ENUM,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET,         METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR,       METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE,   METADATA_STYLE_TYPE_PIXELS,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT,   METADATA_STYLE_TYPE_PLACEMENT_ENUM,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET,      METADATA_STYLE_TYPE_PIXELS,             NULL);

		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR,         METADATA_STYLE_TYPE_COLOR,                "#60a5fa");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH,         METADATA_STYLE_TYPE_PIXELS,              "1");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE,         METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,    "solid");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_START,        METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM, NULL);
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_END,          METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM,   NULL);
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID,          METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM,  "line_end_arrow");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING,  METADATA_STYLE_TYPE_PIXELS,              "12");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE,         METADATA_STYLE_TYPE_PIXELS,              "3");

		cJSON_AddItemToObject(layer, "style", style);
		cJSON_AddItemToArray(layers, layer);
	}

	/* ---- eventList (Point, computeLayer = "eventList", zIndex = 110) --- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",           10);
		cJSON_AddStringToObject(layer, "computeLayer", "eventList");
		cJSON_AddStringToObject(layer, "name",         "Event List");
		cJSON_AddStringToObject(layer, "layerType",    "Point");

		cJSON *point_label_enum_values = cJSON_CreateArray();
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_IN"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_SIDE_IN"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_INIT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_OUT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_SIDE_OUT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("B_DEINIT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("IN"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("SIDE_IN"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("OUT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("SIDE_OUT"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("FLOOR"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("CEILING"));
		cJSON_AddItemToArray(point_label_enum_values, cJSON_CreateString("NONE"));
		cJSON_AddItemToObject(layer, "pointLabelEnumValues", point_label_enum_values);

		cJSON *general = NULL, *point = NULL, *color_mapping = NULL;
		cJSON *style = metadata_create_style_object(&general, &point, NULL, NULL, &color_mapping);

		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE,               METADATA_STYLE_TYPE_BOOLEAN,            "false");
		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX,               METADATA_STYLE_TYPE_INTEGER,            "110");

		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE,             METADATA_STYLE_TYPE_POINT_SHAPE_ENUM,    "circle");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS,            METADATA_STYLE_TYPE_PIXELS,             "4");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING,   METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT,    METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR,      METADATA_STYLE_TYPE_COLOR,               "#a855f7");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH,      METADATA_STYLE_TYPE_PIXELS,             "1");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE,      METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,   "solid");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR,        METADATA_STYLE_TYPE_COLOR,               "#a855f7");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR,          METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE,      METADATA_STYLE_TYPE_PIXELS,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT,    METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT,      METADATA_STYLE_TYPE_PLACEMENT_ENUM,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET,         METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR,       METADATA_STYLE_TYPE_COLOR,               "#a855f7");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE,   METADATA_STYLE_TYPE_PIXELS,      "12");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    "500");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT,   METADATA_STYLE_TYPE_PLACEMENT_ENUM,      "outside-bottom");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET,      METADATA_STYLE_TYPE_PIXELS,             "6");

		metadata_add_point_label_color_entry(color_mapping, "B_IN",       "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "B_SIDE_IN",  "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "B_INIT",     "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "B_OUT",      "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "B_SIDE_OUT", "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "B_DEINIT",   "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "IN",         "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "SIDE_IN",    "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "OUT",        "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "SIDE_OUT",   "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "FLOOR",      "#60a5fa");
		metadata_add_point_label_color_entry(color_mapping, "CEILING",    "#fb923c");
		metadata_add_point_label_color_entry(color_mapping, "NONE",       "#FF00FF");

		cJSON_AddItemToObject(layer, "style", style);
		cJSON_AddItemToArray(layers, layer);
	}

	/* ---- cellList (Polygon, computeLayer = "cellList", zIndex = 120) --- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",           11);
		cJSON_AddStringToObject(layer, "computeLayer", "cellList");
		cJSON_AddStringToObject(layer, "name",         "Cell List");
		cJSON_AddStringToObject(layer, "layerType",    "Polygon");

		cJSON *general = NULL, *point = NULL, *polygon = NULL;
		cJSON *style = metadata_create_style_object(&general, &point, NULL, &polygon, NULL);

		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE,               METADATA_STYLE_TYPE_BOOLEAN,            "false");
		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX,               METADATA_STYLE_TYPE_INTEGER,            "120");

		/* Corner vertex (Point group, Overlap and Text Label excluded) */
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE,          METADATA_STYLE_TYPE_POINT_SHAPE_ENUM,  NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS,         METADATA_STYLE_TYPE_PIXELS,           NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR,   METADATA_STYLE_TYPE_COLOR,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH,   METADATA_STYLE_TYPE_PIXELS,           NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE,   METADATA_STYLE_TYPE_STROKE_STYLE_ENUM, NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR,     METADATA_STYLE_TYPE_COLOR,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR,       METADATA_STYLE_TYPE_COLOR,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE,   METADATA_STYLE_TYPE_PIXELS,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,  NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT,   METADATA_STYLE_TYPE_PLACEMENT_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET,      METADATA_STYLE_TYPE_PIXELS,           NULL);

		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_COLOR,      METADATA_STYLE_TYPE_COLOR,                "#94a3b8");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_WIDTH,      METADATA_STYLE_TYPE_PIXELS,              "1");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_EDGE_STYLE,      METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,    "solid");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_COLOR,      METADATA_STYLE_TYPE_COLOR,                NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_FILL_STYLE,      METADATA_STYLE_TYPE_FILL_STYLE_ENUM,      NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_COLOR,        METADATA_STYLE_TYPE_COLOR,                "#64748b");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_SIZE,    METADATA_STYLE_TYPE_PIXELS,       "12");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FONT_WEIGHT,  METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,     "500");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_SHAPE,        METADATA_STYLE_TYPE_POLYGON_ID_SHAPE_ENUM, NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_RADIUS,       METADATA_STYLE_TYPE_PIXELS,              NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_COLOR, METADATA_STYLE_TYPE_COLOR,                NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_WIDTH, METADATA_STYLE_TYPE_PIXELS,              NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_BORDER_STYLE, METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,    NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_FILL_COLOR,   METADATA_STYLE_TYPE_COLOR,                NULL);
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_PLACEMENT,    METADATA_STYLE_TYPE_PLACEMENT_ENUM,       "inside");
		metadata_add_style_attr(polygon, METADATA_STYLE_ATTR_KEY_POLYGON_ID_OFFSET,       METADATA_STYLE_TYPE_PIXELS,              NULL);

		cJSON_AddItemToObject(layer, "style", style);
		cJSON_AddItemToArray(layers, layer);
	}

	/* ---- Cell Visit Order (Line, computeLayer = "cellVisitOrder", zIndex = 130) --- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",           12);
		cJSON_AddStringToObject(layer, "computeLayer", "cellVisitOrder");
		cJSON_AddStringToObject(layer, "name",         "Cell Visit Order");
		cJSON_AddStringToObject(layer, "layerType",    "Line");

		cJSON *general = NULL, *point = NULL, *line = NULL;
		cJSON *style = metadata_create_style_object(&general, &point, &line, NULL, NULL);

		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_VISIBLE,               METADATA_STYLE_TYPE_BOOLEAN,             "false");
		metadata_add_style_attr(general, METADATA_STYLE_ATTR_KEY_Z_INDEX,               METADATA_STYLE_TYPE_INTEGER,             "130");

		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_SHAPE,             METADATA_STYLE_TYPE_POINT_SHAPE_ENUM,    "circle");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_RADIUS,            METADATA_STYLE_TYPE_PIXELS,             "7");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_SPACING,   METADATA_STYLE_TYPE_PIXELS,             "8");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_OVERLAP_LAYOUT,    METADATA_STYLE_TYPE_OVERLAP_LAYOUT_ENUM, "grid");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_COLOR,      METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_WIDTH,      METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_BORDER_STYLE,      METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,   NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_FILL_COLOR,        METADATA_STYLE_TYPE_COLOR,               "#60a5fa");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_COLOR,          METADATA_STYLE_TYPE_COLOR,               "#ffffff");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_SIZE,      METADATA_STYLE_TYPE_PIXELS,      "12");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_FONT_WEIGHT,    METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    "500");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_PLACEMENT,      METADATA_STYLE_TYPE_PLACEMENT_ENUM,      "inside");
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_ID_OFFSET,         METADATA_STYLE_TYPE_PIXELS,             NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_COLOR,       METADATA_STYLE_TYPE_COLOR,               NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_SIZE,   METADATA_STYLE_TYPE_PIXELS,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_FONT_WEIGHT, METADATA_STYLE_TYPE_FONT_WEIGHT_ENUM,    NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_PLACEMENT,   METADATA_STYLE_TYPE_PLACEMENT_ENUM,      NULL);
		metadata_add_style_attr(point, METADATA_STYLE_ATTR_KEY_POINT_LABEL_OFFSET,      METADATA_STYLE_TYPE_PIXELS,             NULL);

		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_COLOR,         METADATA_STYLE_TYPE_COLOR,               "#60a5fa");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_WIDTH,         METADATA_STYLE_TYPE_PIXELS,             "1");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_EDGE_STYLE,         METADATA_STYLE_TYPE_STROKE_STYLE_ENUM,   "dashed");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_START,        METADATA_STYLE_TYPE_LINE_ARROW_START_ENUM, NULL);
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_END,          METADATA_STYLE_TYPE_LINE_ARROW_END_ENUM,   NULL);
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID,          METADATA_STYLE_TYPE_LINE_ARROW_MID_ENUM,  "line_end_arrow");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_MID_SPACING,  METADATA_STYLE_TYPE_PIXELS,              "20");
		metadata_add_style_attr(line, METADATA_STYLE_ATTR_KEY_LINE_ARROW_SIZE,         METADATA_STYLE_TYPE_PIXELS,              "4");

		cJSON_AddItemToObject(layer, "style", style);
		cJSON_AddItemToArray(layers, layer);
	}

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
