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
 * Appends one style attribute entry { "id": id, "name": name, "value": value|null }
 * to the given style array.  Pass NULL for value to emit a JSON null.
 */
static inline void metadata_add_style_attr(
    cJSON *style_arr,
    int id,
    const char *name,
    const char *value)
{
    cJSON *attr = cJSON_CreateObject();
    cJSON_AddNumberToObject(attr, "id", id);
    cJSON_AddStringToObject(attr, "name", name);
    if (value != NULL)
    {
        cJSON_AddStringToObject(attr, "value", value);
    }
    else
    {
        cJSON_AddNullToObject(attr, "value");
    }
    cJSON_AddItemToArray(style_arr, attr);
}

/*
 * Appends one label enum entry { "value": val, "color": color|null }
 * to the given enumValues array.  Pass NULL for color to emit a JSON null.
 */
static inline void metadata_add_label_enum_value(
    cJSON *enum_values,
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
    cJSON_AddItemToArray(enum_values, entry);
}

#endif // METADATA_JSON_H