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

	metadata_add_parameter(parameters, 1, "Path Width", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "15", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 2, "Path Overlap", BCD_METADATA_PARAM_TYPE_DECIMAL, NULL, 0, "5", BCD_METADATA_PARAM_SECTION_COVERAGE_PATH, BCD_METADATA_APP_HANDLER_NONE);
	metadata_add_parameter(parameters, 3, "Format", BCD_METADATA_PARAM_TYPE_ENUM, format_values, 1, metadata_format_to_string(BCD_METADATA_FORMAT_POLYGON), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_FORMAT);
	metadata_add_parameter(parameters, 4, "Type", BCD_METADATA_PARAM_TYPE_ENUM, type_values, 1, metadata_type_to_string(BCD_METADATA_TYPE_OFFLINE), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_TYPE);
	metadata_add_parameter(parameters, 5, "Coordinate System", BCD_METADATA_PARAM_TYPE_ENUM, coordsystem_values, 1, metadata_coordsystem_to_string(BCD_METADATA_COORDSYSTEM_DECIMAL), BCD_METADATA_PARAM_SECTION_ENVIRONMENT, BCD_METADATA_APP_HANDLER_ENVIRONMENT_COORDSYSTEM);

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

		cJSON *universal = NULL, *point = NULL, *line = NULL;
		cJSON *style = metadata_create_style_object(&universal, &point, &line, NULL, NULL);

		metadata_add_style_attr(universal, "Z-Index", "Integer", "100");

		metadata_add_style_attr(point, "Point Shape",             "PointShapeEnum",    NULL);
		metadata_add_style_attr(point, "Point Radius",            "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Overlap Spacing",   "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Overlap Layout",    "OverlapLayoutEnum", NULL);
		metadata_add_style_attr(point, "Point Border Color",      "Color",             NULL);
		metadata_add_style_attr(point, "Point Border Width",      "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Border Style",      "StrokeStyleEnum",   NULL);
		metadata_add_style_attr(point, "Point Fill Color",        "Color",             NULL);
		metadata_add_style_attr(point, "Point ID Color",          "Color",             NULL);
		metadata_add_style_attr(point, "Point ID Font Size",      "FontSizeEnum",      NULL);
		metadata_add_style_attr(point, "Point ID Font Weight",    "FontWeightEnum",    NULL);
		metadata_add_style_attr(point, "Point ID Placement",      "PlacementEnum",     NULL);
		metadata_add_style_attr(point, "Point ID Offset",         "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Label Color",       "Color",             NULL);
		metadata_add_style_attr(point, "Point Label Font Size",   "FontSizeEnum",      NULL);
		metadata_add_style_attr(point, "Point Label Font Weight", "FontWeightEnum",    NULL);
		metadata_add_style_attr(point, "Point Label Placement",   "PlacementEnum",     NULL);
		metadata_add_style_attr(point, "Point Label Offset",      "Spacing",           NULL);

		metadata_add_style_attr(line, "Line Edge Color",         "Color",             "#60a5fa");
		metadata_add_style_attr(line, "Line Edge Width",         "Spacing",           "1");
		metadata_add_style_attr(line, "Line Edge Style",         "StrokeStyleEnum",   "solid");
		metadata_add_style_attr(line, "Line Arrow Start",        "LineArrowStartEnum", NULL);
		metadata_add_style_attr(line, "Line Arrow End",          "LineArrowEndEnum",   NULL);
		metadata_add_style_attr(line, "Line Arrow Mid",          "LineArrowMidEnum",  "line_end_arrow");
		metadata_add_style_attr(line, "Line Arrow Mid Spacing",  "Spacing",           "12");
		metadata_add_style_attr(line, "Line Arrow Size",         "Spacing",           "3");

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

		cJSON *universal = NULL, *point = NULL, *color_mapping = NULL;
		cJSON *style = metadata_create_style_object(&universal, &point, NULL, NULL, &color_mapping);

		metadata_add_style_attr(universal, "Z-Index", "Integer", "110");

		metadata_add_style_attr(point, "Point Shape",             "PointShapeEnum",    "circle");
		metadata_add_style_attr(point, "Point Radius",            "Spacing",           "4");
		metadata_add_style_attr(point, "Point Overlap Spacing",   "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Overlap Layout",    "OverlapLayoutEnum", NULL);
		metadata_add_style_attr(point, "Point Border Color",      "Color",             "#a855f7");
		metadata_add_style_attr(point, "Point Border Width",      "Spacing",           "1");
		metadata_add_style_attr(point, "Point Border Style",      "StrokeStyleEnum",   "solid");
		metadata_add_style_attr(point, "Point Fill Color",        "Color",             "#a855f7");
		metadata_add_style_attr(point, "Point ID Color",          "Color",             NULL);
		metadata_add_style_attr(point, "Point ID Font Size",      "FontSizeEnum",      NULL);
		metadata_add_style_attr(point, "Point ID Font Weight",    "FontWeightEnum",    NULL);
		metadata_add_style_attr(point, "Point ID Placement",      "PlacementEnum",     NULL);
		metadata_add_style_attr(point, "Point ID Offset",         "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Label Color",       "Color",             "#a855f7");
		metadata_add_style_attr(point, "Point Label Font Size",   "FontSizeEnum",      "text-xs");
		metadata_add_style_attr(point, "Point Label Font Weight", "FontWeightEnum",    "font-medium");
		metadata_add_style_attr(point, "Point Label Placement",   "PlacementEnum",     "outside-bottom");
		metadata_add_style_attr(point, "Point Label Offset",      "Spacing",           "6");

		metadata_add_point_label_color_entry(color_mapping, "B_IN",       "#16a34a");
		metadata_add_point_label_color_entry(color_mapping, "B_SIDE_IN",  "#0d9488");
		metadata_add_point_label_color_entry(color_mapping, "B_INIT",     "#bbf7d0");
		metadata_add_point_label_color_entry(color_mapping, "B_OUT",      "#dc2626");
		metadata_add_point_label_color_entry(color_mapping, "B_SIDE_OUT", "#db2777");
		metadata_add_point_label_color_entry(color_mapping, "B_DEINIT",   "#fecaca");
		metadata_add_point_label_color_entry(color_mapping, "IN",         "#4ade80");
		metadata_add_point_label_color_entry(color_mapping, "SIDE_IN",    "#2dd4bf");
		metadata_add_point_label_color_entry(color_mapping, "OUT",        "#f87171");
		metadata_add_point_label_color_entry(color_mapping, "SIDE_OUT",   "#f472b6");
		metadata_add_point_label_color_entry(color_mapping, "FLOOR",      "#60a5fa");
		metadata_add_point_label_color_entry(color_mapping, "CEILING",    "#fb923c");
		metadata_add_point_label_color_entry(color_mapping, "NONE",       NULL);

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

		cJSON *universal = NULL, *point = NULL, *polygon = NULL;
		cJSON *style = metadata_create_style_object(&universal, &point, NULL, &polygon, NULL);

		metadata_add_style_attr(universal, "Z-Index", "Integer", "120");

		/* Corner vertex (Point group, Overlap and Text Label excluded) */
		metadata_add_style_attr(point, "Point Shape",          "PointShapeEnum",  NULL);
		metadata_add_style_attr(point, "Point Radius",         "Spacing",         NULL);
		metadata_add_style_attr(point, "Point Border Color",   "Color",           NULL);
		metadata_add_style_attr(point, "Point Border Width",   "Spacing",         NULL);
		metadata_add_style_attr(point, "Point Border Style",   "StrokeStyleEnum", NULL);
		metadata_add_style_attr(point, "Point Fill Color",     "Color",           NULL);
		metadata_add_style_attr(point, "Point ID Color",       "Color",           NULL);
		metadata_add_style_attr(point, "Point ID Font Size",   "FontSizeEnum",    NULL);
		metadata_add_style_attr(point, "Point ID Font Weight", "FontWeightEnum",  NULL);
		metadata_add_style_attr(point, "Point ID Placement",   "PlacementEnum",   NULL);
		metadata_add_style_attr(point, "Point ID Offset",      "Spacing",         NULL);

		metadata_add_style_attr(polygon, "Polygon Edge Color",      "Color",             "#94a3b8");
		metadata_add_style_attr(polygon, "Polygon Edge Width",      "Spacing",           "1");
		metadata_add_style_attr(polygon, "Polygon Edge Style",      "StrokeStyleEnum",   "solid");
		metadata_add_style_attr(polygon, "Polygon Fill Color",      "Color",             NULL);
		metadata_add_style_attr(polygon, "Polygon Fill Style",      "FillStyleEnum",     NULL);
		metadata_add_style_attr(polygon, "Polygon ID Color",        "Color",             "#64748b");
		metadata_add_style_attr(polygon, "Polygon ID Font Size",    "FontSizeEnum",      "text-xs");
		metadata_add_style_attr(polygon, "Polygon ID Font Weight",  "FontWeightEnum",    "font-medium");
		metadata_add_style_attr(polygon, "Polygon ID Shape",        "PolygonIDShapeEnum", NULL);
		metadata_add_style_attr(polygon, "Polygon ID Radius",       "Spacing",           NULL);
		metadata_add_style_attr(polygon, "Polygon ID Border Color", "Color",             NULL);
		metadata_add_style_attr(polygon, "Polygon ID Border Width", "Spacing",           NULL);
		metadata_add_style_attr(polygon, "Polygon ID Border Style", "StrokeStyleEnum",   NULL);
		metadata_add_style_attr(polygon, "Polygon ID Fill Color",   "Color",             NULL);
		metadata_add_style_attr(polygon, "Polygon ID Placement",    "PlacementEnum",     "inside");
		metadata_add_style_attr(polygon, "Polygon ID Offset",       "Spacing",           NULL);

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

		cJSON *universal = NULL, *point = NULL, *line = NULL;
		cJSON *style = metadata_create_style_object(&universal, &point, &line, NULL, NULL);

		metadata_add_style_attr(universal, "Z-Index", "Integer", "130");

		metadata_add_style_attr(point, "Point Shape",             "PointShapeEnum",    "circle");
		metadata_add_style_attr(point, "Point Radius",            "Spacing",           "3");
		metadata_add_style_attr(point, "Point Overlap Spacing",   "Spacing",           "8");
		metadata_add_style_attr(point, "Point Overlap Layout",    "OverlapLayoutEnum", "grid");
		metadata_add_style_attr(point, "Point Border Color",      "Color",             NULL);
		metadata_add_style_attr(point, "Point Border Width",      "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Border Style",      "StrokeStyleEnum",   NULL);
		metadata_add_style_attr(point, "Point Fill Color",        "Color",             "#60a5fa");
		metadata_add_style_attr(point, "Point ID Color",          "Color",             "#60a5fa");
		metadata_add_style_attr(point, "Point ID Font Size",      "FontSizeEnum",      "text-xs");
		metadata_add_style_attr(point, "Point ID Font Weight",    "FontWeightEnum",    "font-medium");
		metadata_add_style_attr(point, "Point ID Placement",      "PlacementEnum",     "inside");
		metadata_add_style_attr(point, "Point ID Offset",         "Spacing",           NULL);
		metadata_add_style_attr(point, "Point Label Color",       "Color",             NULL);
		metadata_add_style_attr(point, "Point Label Font Size",   "FontSizeEnum",      NULL);
		metadata_add_style_attr(point, "Point Label Font Weight", "FontWeightEnum",    NULL);
		metadata_add_style_attr(point, "Point Label Placement",   "PlacementEnum",     NULL);
		metadata_add_style_attr(point, "Point Label Offset",      "Spacing",           NULL);

		metadata_add_style_attr(line, "Line Edge Color",         "Color",              "#60a5fa");
		metadata_add_style_attr(line, "Line Edge Width",         "Spacing",            "1");
		metadata_add_style_attr(line, "Line Edge Style",         "StrokeStyleEnum",    "solid");
		metadata_add_style_attr(line, "Line Arrow Start",        "LineArrowStartEnum", NULL);
		metadata_add_style_attr(line, "Line Arrow End",          "LineArrowEndEnum",   NULL);
		metadata_add_style_attr(line, "Line Arrow Mid",          "LineArrowMidEnum",   "line_end_arrow");
		metadata_add_style_attr(line, "Line Arrow Mid Spacing",  "Spacing",            "12");
		metadata_add_style_attr(line, "Line Arrow Size",         "Spacing",            "3");

		cJSON_AddItemToObject(layer, "style", style);
		cJSON_AddItemToArray(layers, layer);
	}

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
