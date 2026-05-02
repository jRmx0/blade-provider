#include "bounce.h"

#include "../../../dependencies/cJSON/cJSON.h"

static char *bounce_create_error_json(const char *code, const char *message)
{
    cJSON *response = cJSON_CreateObject();
    if (response == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "code", code);
    cJSON_AddStringToObject(response, "message", message);

    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    return json;
}

char *bounce_get_metadata_json(void)
{
    cJSON *algorithm = cJSON_CreateObject();
    if (algorithm == NULL)
    {
        return NULL;
    }

    cJSON_AddNumberToObject(algorithm, "id", 2);
    cJSON_AddStringToObject(algorithm, "name", "Bounce");

    cJSON *parameters = cJSON_CreateArray();
    cJSON *layers = cJSON_CreateArray();
    cJSON *metrics = cJSON_CreateArray();
    if (parameters == NULL || layers == NULL || metrics == NULL)
    {
        cJSON_Delete(parameters);
        cJSON_Delete(layers);
        cJSON_Delete(metrics);
        cJSON_Delete(algorithm);
        return NULL;
    }

    cJSON_AddItemToObject(algorithm, "parameters", parameters);
    cJSON_AddItemToObject(algorithm, "layers", layers);
    cJSON_AddItemToObject(algorithm, "metrics", metrics);

    char *json = cJSON_PrintUnformatted(algorithm);
    cJSON_Delete(algorithm);
    return json;
}

char *bounce_compute(const char *input_environment_json)
{
    (void)input_environment_json;
    return bounce_create_error_json(
        "not_implemented",
        "Bounce algorithm Stage 1 wire-up is complete, but compute is not implemented yet.");
}