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

    if (section_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "section", section_value);
    }

    if (app_handler_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "appHandler", app_handler_value);
    }

    cJSON_AddItemToArray(parameters, parameter);
}

static inline void metadata_add_debug_layer(
    cJSON *debug_layers,
    int id,
    const char *key,
    const char *name,
    const char *class_name,
    int point_radius,
    int font_size)
{
    cJSON *layer = cJSON_CreateObject();
    cJSON_AddNumberToObject(layer, "id", id);
    cJSON_AddStringToObject(layer, "key", key);
    cJSON_AddStringToObject(layer, "name", name);

    cJSON *style = cJSON_CreateObject();
    cJSON_AddStringToObject(style, "className", class_name);
    if (point_radius >= 0)
    {
        cJSON_AddNumberToObject(style, "pointRadius", point_radius);
    }
    if (font_size >= 0)
    {
        cJSON_AddNumberToObject(style, "fontSize", font_size);
    }
    cJSON_AddItemToObject(layer, "style", style);

    cJSON_AddItemToArray(debug_layers, layer);
}

#endif // METADATA_JSON_H