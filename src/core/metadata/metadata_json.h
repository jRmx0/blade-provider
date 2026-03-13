#ifndef METADATA_JSON_H
#define METADATA_JSON_H

#include "metadata_types.h"
#include "../../../dependencies/cJSON/cjson_compat.h"

static inline void metadata_add_parameter(
    cJSON *parameters,
    const char *name,
    const char *label,
    bcd_metadata_param_type_t param_type,
    const char *default_value,
    const char *const *enum_values,
    int enum_value_count)
{
    cJSON *parameter = cJSON_CreateObject();
    cJSON_AddStringToObject(parameter, "name", name);
    cJSON_AddStringToObject(parameter, "label", label);
    cJSON_AddStringToObject(parameter, "paramType", metadata_param_type_to_string(param_type));

    if (default_value != NULL)
    {
        cJSON_AddStringToObject(parameter, "defaultValue", default_value);
    }

    if (enum_values != NULL && enum_value_count > 0)
    {
        cJSON *enum_array = cJSON_CreateArray();
        for (int i = 0; i < enum_value_count; ++i)
        {
            cJSON_AddItemToArray(enum_array, cJSON_CreateString(enum_values[i]));
        }
        cJSON_AddItemToObject(parameter, "enumValues", enum_array);
    }

    cJSON_AddItemToArray(parameters, parameter);
}

#endif // METADATA_JSON_H