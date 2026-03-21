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

	cJSON *debug_layers = cJSON_CreateArray();
	if (debug_layers == NULL)
	{
		cJSON_Delete(algorithm);
		return NULL;
	}
	cJSON_AddItemToObject(algorithm, "debugLayers", debug_layers);

	/* ---- eventList (Point, zIndex = 1) -------------------------------- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",     1);
		cJSON_AddStringToObject(layer, "debugKey", "eventList");
		cJSON_AddStringToObject(layer, "name",   "Event List");
		cJSON_AddStringToObject(layer, "type",   "Point");

		cJSON *style = cJSON_CreateArray();
		metadata_add_style_attr(style, 1,  "Z-Index",             "100");
		metadata_add_style_attr(style, 10, "Point Shape",             "circle");
		metadata_add_style_attr(style, 11, "Point Radius",            "4");
		metadata_add_style_attr(style, 12, "Point Overlap Spacing",   NULL);
		metadata_add_style_attr(style, 13, "Point Overlap Layout",    NULL);
		metadata_add_style_attr(style, 20, "Point Border Color",      "purple-500");
		metadata_add_style_attr(style, 21, "Point Border Width",      "1");
		metadata_add_style_attr(style, 22, "Point Border Style",      "solid");
		metadata_add_style_attr(style, 30, "Point Fill Color",        "purple-500");
		metadata_add_style_attr(style, 40, "Point ID Color",          NULL);
		metadata_add_style_attr(style, 41, "Point ID Font Size",      NULL);
		metadata_add_style_attr(style, 42, "Point ID Font Weight",    NULL);
		metadata_add_style_attr(style, 43, "Point ID Placement",      NULL);
		metadata_add_style_attr(style, 44, "Point ID Offset",         NULL);
		metadata_add_style_attr(style, 50, "Point Label Placement",   "outside-bottom");
		metadata_add_style_attr(style, 51, "Point Label Color",       "purple-500");
		metadata_add_style_attr(style, 52, "Point Label Font Size",   "text-xs");
		metadata_add_style_attr(style, 53, "Point Label Font Weight", "font-medium");
		metadata_add_style_attr(style, 54, "Point Label Offset",      "6");
		cJSON_AddItemToObject(layer, "style", style);

		cJSON *label     = cJSON_CreateObject();
		cJSON *enum_vals = cJSON_CreateArray();
		cJSON_AddStringToObject(label, "key", "eventType");
		metadata_add_label_enum_value(enum_vals, "B_IN",       "green-600");
		metadata_add_label_enum_value(enum_vals, "B_SIDE_IN",  "teal-600");
		metadata_add_label_enum_value(enum_vals, "B_INIT",     "green-200");
		metadata_add_label_enum_value(enum_vals, "B_OUT",      "red-600");
		metadata_add_label_enum_value(enum_vals, "B_SIDE_OUT", "pink-600");
		metadata_add_label_enum_value(enum_vals, "B_DEINIT",   "red-200");
		metadata_add_label_enum_value(enum_vals, "IN",         "green-400");
		metadata_add_label_enum_value(enum_vals, "SIDE_IN",    "teal-400");
		metadata_add_label_enum_value(enum_vals, "OUT",        "red-400");
		metadata_add_label_enum_value(enum_vals, "SIDE_OUT",   "pink-400");
		metadata_add_label_enum_value(enum_vals, "FLOOR",      "blue-400");
		metadata_add_label_enum_value(enum_vals, "CEILING",    "orange-400");
		metadata_add_label_enum_value(enum_vals, "NONE",       NULL);
		cJSON_AddItemToObject(label, "enumValues", enum_vals);
		cJSON_AddItemToObject(layer, "label", label);

		cJSON_AddItemToArray(debug_layers, layer);
	}

	/* ---- cellList (Polygon, zIndex = 2) ------------------------------- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",     2);
		cJSON_AddStringToObject(layer, "debugKey", "cellList");
		cJSON_AddStringToObject(layer, "name",   "Cell List");
		cJSON_AddStringToObject(layer, "type",   "Polygon");

		cJSON *style = cJSON_CreateArray();
		metadata_add_style_attr(style, 1,  "Z-Index",             "110");
		/* Corner vertex (10-44, excluding overlap 12-13 and text label 50-54) */
		metadata_add_style_attr(style, 10, "Point Shape",             NULL);
		metadata_add_style_attr(style, 11, "Point Radius",            NULL);
		metadata_add_style_attr(style, 20, "Point Border Color",      NULL);
		metadata_add_style_attr(style, 21, "Point Border Width",      NULL);
		metadata_add_style_attr(style, 22, "Point Border Style",      NULL);
		metadata_add_style_attr(style, 30, "Point Fill Color",        NULL);
		metadata_add_style_attr(style, 40, "Point ID Color",          NULL);
		metadata_add_style_attr(style, 41, "Point ID Font Size",      NULL);
		metadata_add_style_attr(style, 42, "Point ID Font Weight",    NULL);
		metadata_add_style_attr(style, 43, "Point ID Placement",      NULL);
		metadata_add_style_attr(style, 44, "Point ID Offset",         NULL);
		/* Polygon Edge */
		metadata_add_style_attr(style, 60, "Polygon Edge Color",      "slate-400");
		metadata_add_style_attr(style, 61, "Polygon Edge Width",      "1");
		metadata_add_style_attr(style, 62, "Polygon Edge Style",      "solid");
		/* Polygon Fill */
		metadata_add_style_attr(style, 80, "Polygon Fill Color",      NULL);
		metadata_add_style_attr(style, 81, "Polygon Fill Style",      NULL);
		/* Polygon ID */
		metadata_add_style_attr(style, 82, "Polygon ID Color",        "slate-500");
		metadata_add_style_attr(style, 83, "Polygon ID Font Size",    "text-xs");
		metadata_add_style_attr(style, 84, "Polygon ID Font Weight",  "font-medium");
		metadata_add_style_attr(style, 85, "Polygon ID Shape",        NULL);
		metadata_add_style_attr(style, 86, "Polygon ID Radius",       NULL);
		metadata_add_style_attr(style, 87, "Polygon ID Border Color", NULL);
		metadata_add_style_attr(style, 88, "Polygon ID Border Width", NULL);
		metadata_add_style_attr(style, 89, "Polygon ID Border Style", NULL);
		metadata_add_style_attr(style, 90, "Polygon ID Fill Color",   NULL);
		metadata_add_style_attr(style, 91, "Polygon ID Placement",    "inside");
		metadata_add_style_attr(style, 92, "Polygon ID Offset",       NULL);
		cJSON_AddItemToObject(layer, "style", style);

		cJSON_AddNullToObject(layer, "label");

		cJSON_AddItemToArray(debug_layers, layer);
	}

	/* ---- cellVisitOrder (Line, zIndex = 3) ---------------------------- */
	{
		cJSON *layer = cJSON_CreateObject();
		cJSON_AddNumberToObject(layer, "id",     3);
		cJSON_AddStringToObject(layer, "debugKey", "cellVisitOrder");
		cJSON_AddStringToObject(layer, "name",   "Cell Visit Order");
		cJSON_AddStringToObject(layer, "type",   "Line");

		cJSON *style = cJSON_CreateArray();
		metadata_add_style_attr(style, 1,  "Z-Index",             "120");
		/* Point vertex (all 18 Point attributes) */
		metadata_add_style_attr(style, 10, "Point Shape",             "circle");
		metadata_add_style_attr(style, 11, "Point Radius",            "3");
		metadata_add_style_attr(style, 12, "Point Overlap Spacing",   "8");
		metadata_add_style_attr(style, 13, "Point Overlap Layout",    "grid");
		metadata_add_style_attr(style, 20, "Point Border Color",      NULL);
		metadata_add_style_attr(style, 21, "Point Border Width",      NULL);
		metadata_add_style_attr(style, 22, "Point Border Style",      NULL);
		metadata_add_style_attr(style, 30, "Point Fill Color",        "blue-400");
		metadata_add_style_attr(style, 40, "Point ID Color",          "blue-400");
		metadata_add_style_attr(style, 41, "Point ID Font Size",      "text-xs");
		metadata_add_style_attr(style, 42, "Point ID Font Weight",    "font-medium");
		metadata_add_style_attr(style, 43, "Point ID Placement",      "inside");
		metadata_add_style_attr(style, 44, "Point ID Offset",         NULL);
		metadata_add_style_attr(style, 50, "Point Label Placement",   NULL);
		metadata_add_style_attr(style, 51, "Point Label Color",       NULL);
		metadata_add_style_attr(style, 52, "Point Label Font Size",   NULL);
		metadata_add_style_attr(style, 53, "Point Label Font Weight", NULL);
		metadata_add_style_attr(style, 54, "Point Label Offset",      NULL);
		/* Line Edge */
		metadata_add_style_attr(style, 60, "Line Edge Color",         "blue-400");
		metadata_add_style_attr(style, 61, "Line Edge Width",         "1");
		metadata_add_style_attr(style, 62, "Line Edge Style",         "solid");
		/* Arrow */
		metadata_add_style_attr(style, 70, "Line Arrow Start",        NULL);
		metadata_add_style_attr(style, 71, "Line Arrow End",          NULL);
		metadata_add_style_attr(style, 72, "Line Arrow Mid",          "line_end_arrow");
		metadata_add_style_attr(style, 73, "Line Arrow Mid Spacing",  "12");
		metadata_add_style_attr(style, 74, "Line Arrow Size",         "3");
		cJSON_AddItemToObject(layer, "style", style);

		cJSON_AddNullToObject(layer, "label");

		cJSON_AddItemToArray(debug_layers, layer);
	}

	char *json = cJSON_PrintUnformatted(algorithm);
	cJSON_Delete(algorithm);
	return json;
}
