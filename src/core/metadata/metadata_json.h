#ifndef METADATA_JSON_H
#define METADATA_JSON_H

#include "../../../dependencies/cJSON/cjson.h"

static inline void metadata_add_parameter(
    cJSON *parameters,
    int id,
    const char *name,
    const char *param_type,
    const char *const *enum_values,
    int enum_value_count,
    const char *default_value,
    const char *section,
    const char *app_handler,
    int has_min_value,
    double min_value,
    int has_max_value,
    double max_value,
    const char *unit_type)
{
    cJSON *parameter = cJSON_CreateObject();

    cJSON_AddNumberToObject(parameter, "id", id);

    cJSON_AddStringToObject(parameter, "name", name);
    cJSON_AddStringToObject(parameter, "paramType", param_type);

    if (enum_values != NULL && enum_value_count > 0)
    {
        cJSON *enum_array = cJSON_CreateArray();
        for (int i = 0; i < enum_value_count; ++i)
        {
            cJSON_AddItemToArray(enum_array, cJSON_CreateString(enum_values[i]));
        }
        cJSON_AddItemToObject(parameter, "enumValues", enum_array);
    }

    if (default_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "defaultValue", default_value);
    }

    if (has_min_value)
    {
        cJSON_AddNumberToObject(parameter, "minValue", min_value);
    }

    if (has_max_value)
    {
        cJSON_AddNumberToObject(parameter, "maxValue", max_value);
    }

    if (section != NULL)
    {
        cJSON_AddStringToObject(parameter, "section", section);
    }

    if (app_handler != NULL)
    {
        cJSON_AddStringToObject(parameter, "appHandler", app_handler);
    }

    if (unit_type != NULL)
    {
        cJSON_AddStringToObject(parameter, "unitType", unit_type);
    }

    cJSON_AddItemToArray(parameters, parameter);
}

/*
 * Appends one performance metric entry { "id": id, "name": name, "type": type [, "group": group] }
 * to the given metrics array.  Pass NULL for group to omit the field.
 */
static inline cJSON *metadata_add_metric(
    cJSON *metrics,
    int id,
    const char *name,
    const char *type,
    const char *group)
{
    cJSON *metric = cJSON_CreateObject();
    cJSON_AddNumberToObject(metric, "id", id);
    cJSON_AddStringToObject(metric, "name", name);
    cJSON_AddStringToObject(metric, "type", type);
    if (group != NULL)
        cJSON_AddStringToObject(metric, "group", group);
    cJSON_AddItemToArray(metrics, metric);
    return metric;
}

/*
 * Attaches a "style" sub-object to a Time-series metric with optional axis labels.
 * Pass NULL for either label to omit that field.
 * Call this immediately after metadata_add_metric for Time-series metrics.
 */
static inline void metadata_add_timeseries_style(
    cJSON *metric,
    const char *x_axis_label,
    const char *y_axis_label)
{
    cJSON *style = cJSON_CreateObject();
    if (x_axis_label != NULL)
        cJSON_AddStringToObject(style, "xAxisLabel", x_axis_label);
    if (y_axis_label != NULL)
        cJSON_AddStringToObject(style, "yAxisLabel", y_axis_label);
    cJSON_AddItemToObject(metric, "style", style);
}

/*
 * Attaches a "style" sub-object to a Bar-chart metric with optional axis labels.
 * Pass NULL for either label to omit that field.
 * Call this immediately after metadata_add_metric for Bar-chart metrics.
 */
static inline void metadata_add_barchart_style(
    cJSON *metric,
    const char *x_axis_label,
    const char *y_axis_label)
{
    cJSON *style = cJSON_CreateObject();
    if (x_axis_label != NULL)
        cJSON_AddStringToObject(style, "xAxisLabel", x_axis_label);
    if (y_axis_label != NULL)
        cJSON_AddStringToObject(style, "yAxisLabel", y_axis_label);
    cJSON_AddItemToObject(metric, "style", style);
}

/*
 * Appends one style attribute entry { "key": key, "styleType": style_type, "defaultValue": value|null }
 * to the given style sub-array.  Pass NULL for value to emit a JSON null.
 */
static inline void metadata_add_style_attr(
    cJSON *style_arr,
    const char *key,
    const char *style_type,
    const char *value)
{
    cJSON *attr = cJSON_CreateObject();
    cJSON_AddStringToObject(attr, "key", key);
    cJSON_AddStringToObject(attr, "styleType", style_type);
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
 * Callers must add the appropriate sub-arrays using metadata_style_general(),
 * metadata_style_point(), metadata_style_line(), and metadata_style_polygon().
 *
 * For Point layers:   generalStyleAttributes + pointStyleAttributes
 * For Line layers:    generalStyleAttributes + pointStyleAttributes + lineStyleAttributes
 * For Polygon layers: generalStyleAttributes + pointStyleAttributes + polygonStyleAttributes
 */
static inline cJSON *metadata_create_style_object(
    cJSON **out_general,
    cJSON **out_point,
    cJSON **out_line,
    cJSON **out_polygon,
    cJSON **out_point_label_color_mapping)
{
    cJSON *style = cJSON_CreateObject();
    *out_general = cJSON_CreateArray();
    cJSON_AddItemToObject(style, "generalStyleAttributes", *out_general);
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