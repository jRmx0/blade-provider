#ifndef METADATA_JSON_H
#define METADATA_JSON_H

#include "metadata_types.h"
#include "../../../dependencies/cJSON/cjson.h"

static inline void metadata_add_parameter(
    cJSON *parameters,
    int id,
    const char *name,
    bcd_metadata_param_type_t param_type,
    const char *const *enum_values,
    int enum_value_count,
    const char *default_value,
    bcd_metadata_param_section_t section,
    bcd_metadata_app_handler_t app_handler)
{
    cJSON *parameter = cJSON_CreateObject();
    const char *section_value = metadata_param_section_to_string(section);
    const char *app_handler_value = metadata_app_handler_to_string(app_handler);

    cJSON_AddNumberToObject(parameter, "id", id);

    cJSON_AddStringToObject(parameter, "name", name);
    cJSON_AddStringToObject(parameter, "paramType", metadata_param_type_to_string(param_type));

    cJSON *enum_array = cJSON_CreateArray();
    if (enum_values != NULL && enum_value_count > 0)
    {
        for (int i = 0; i < enum_value_count; ++i)
        {
            cJSON_AddItemToArray(enum_array, cJSON_CreateString(enum_values[i]));
        }
    }
    cJSON_AddItemToObject(parameter, "enumValues", enum_array);

    if (default_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "defaultValue", default_value);
    }

    if (section_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "section", section_value);
    }

    if (app_handler_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "appHandler", app_handler_value);
    }
    else
    {
        cJSON_AddNullToObject(parameter, "appHandler");
    }

    cJSON_AddItemToArray(parameters, parameter);
}

/*
 * Appends one style attribute entry { "key": key, "styleType": style_type, "defaultValue": value|null }
 * to the given style sub-array.  Pass NULL for value to emit a JSON null.
 */
static inline void metadata_add_style_attr(
    cJSON *style_arr,
    metadata_style_attr_key_t key,
    metadata_style_type_t style_type,
    const char *value)
{
    cJSON *attr = cJSON_CreateObject();
    cJSON_AddStringToObject(attr, "key", metadata_style_attr_key_to_string(key));
    cJSON_AddStringToObject(attr, "styleType", metadata_style_type_to_string(style_type));
    if (value != NULL)
    {
        cJSON_AddStringToObject(attr, "defaultValue", value);
    }
    else
    {
        cJSON_AddNullToObject(attr, "defaultValue");
    }
    cJSON_AddItemToArray(style_arr, attr);
}

/*
 * Creates and returns a LayerStyle object pre-populated with empty sub-arrays.
 * Callers must add the appropriate sub-arrays using metadata_style_universal(),
 * metadata_style_point(), metadata_style_line(), and metadata_style_polygon().
 *
 * For Point layers:   universalStyleAttributes + pointStyleAttributes
 * For Line layers:    universalStyleAttributes + pointStyleAttributes + lineStyleAttributes
 * For Polygon layers: universalStyleAttributes + pointStyleAttributes + polygonStyleAttributes
 */
static inline cJSON *metadata_create_style_object(
    cJSON **out_universal,
    cJSON **out_point,
    cJSON **out_line,
    cJSON **out_polygon,
    cJSON **out_point_label_color_mapping)
{
    cJSON *style = cJSON_CreateObject();
    *out_universal = cJSON_CreateArray();
    cJSON_AddItemToObject(style, "universalStyleAttributes", *out_universal);
    if (out_point != NULL)
    {
        *out_point = cJSON_CreateArray();
        cJSON_AddItemToObject(style, "pointStyleAttributes", *out_point);
    }
    if (out_line != NULL)
    {
        *out_line = cJSON_CreateArray();
        cJSON_AddItemToObject(style, "lineStyleAttributes", *out_line);
    }
    if (out_polygon != NULL)
    {
        *out_polygon = cJSON_CreateArray();
        cJSON_AddItemToObject(style, "polygonStyleAttributes", *out_polygon);
    }
    if (out_point_label_color_mapping != NULL)
    {
        *out_point_label_color_mapping = cJSON_CreateArray();
        cJSON_AddItemToObject(style, "pointLabelColorMapping", *out_point_label_color_mapping);
    }
    return style;
}

/*
 * Appends one point-label color entry { "value": val, "color": color|null }
 * to the given pointLabelColorMapping array.  Pass NULL for color to emit JSON null.
 */
static inline void metadata_add_point_label_color_entry(
    cJSON *mapping_arr,
    const char *value,
    const char *color)
{
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddStringToObject(entry, "value", value);
    if (color != NULL)
    {
        cJSON_AddStringToObject(entry, "color", color);
    }
    else
    {
        cJSON_AddNullToObject(entry, "color");
    }
    cJSON_AddItemToArray(mapping_arr, entry);
}

#endif // METADATA_JSON_H