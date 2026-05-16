/**
 * debug_serialize.c
 *
 * Implementation of the shared debug-segment serialiser.
 * See debug_serialize.h for the public contract.
 */

#include "debug_serialize.h"
#include <stddef.h>

cJSON *debug_build_segment(int id, const char *seg_type,
                           const float *xs, const float *ys, int count)
{
    cJSON *jsegment = cJSON_CreateObject();
    cJSON *path_arr = cJSON_CreateArray();

    if (jsegment == NULL || path_arr == NULL)
    {
        cJSON_Delete(jsegment);
        cJSON_Delete(path_arr);
        return NULL;
    }

    cJSON_AddNumberToObject(jsegment, "id", id);
    cJSON_AddStringToObject(jsegment, "type", seg_type != NULL ? seg_type : "coverage");
    cJSON_AddItemToObject(jsegment, "path", path_arr);

    for (int i = 0; i < count; ++i)
    {
        cJSON *jentry = cJSON_CreateObject();
        cJSON *jpoint = cJSON_CreateObject();

        if (jentry == NULL || jpoint == NULL)
        {
            cJSON_Delete(jentry);
            cJSON_Delete(jpoint);
            /* path_arr is already owned by jsegment — jsegment will be freed by caller */
            continue;
        }

        cJSON_AddNumberToObject(jentry, "id", i);
        cJSON_AddNumberToObject(jpoint, "x", xs[i]);
        cJSON_AddNumberToObject(jpoint, "y", ys[i]);
        cJSON_AddItemToObject(jentry, "point", jpoint);
        cJSON_AddItemToArray(path_arr, jentry);
    }

    return jsegment;
}
