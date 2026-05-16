#include "../../../../dependencies/cJSON/cJSON.h"
#include <string.h>
#include "../internal.h"
#include "../check/cstar_check.h"
#include "runner/cstar_runner.h"

#include "../check/cstar_check.c"
#include "runner/cstar_runner.c"

static char *cstar_create_error_json(const char *code, const char *message)
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

char *cstar_run_compute(const char *input_environment_json)
{
    input_environment_t environment;
    cstar_check_result_t check_result;
    if (!cstar_check_request_json(input_environment_json, &environment, &check_result))
    {
        return cstar_create_error_json(check_result.code, check_result.message);
    }

    cJSON *root = cstar_coverage_path_planning_process(&environment);
    cstar_check_free_environment(&environment);

    if (root == NULL)
    {
        return cstar_create_error_json("allocation_failed", "C* compute pipeline allocation failed.");
    }

    cJSON *status_field = cJSON_GetObjectItemCaseSensitive(root, "status");
    if (status_field != NULL && cJSON_IsString(status_field) && strcmp(status_field->valuestring, "error") == 0)
    {
        const cJSON *code_field = cJSON_GetObjectItemCaseSensitive(root, "code");
        const cJSON *message_field = cJSON_GetObjectItemCaseSensitive(root, "message");

        const char *code = (code_field != NULL && cJSON_IsString(code_field)) ? code_field->valuestring : "cstar_error";
        const char *message = (message_field != NULL && cJSON_IsString(message_field)) ? message_field->valuestring : "C* compute pipeline failed.";

        char *json = cstar_create_error_json(code, message);
        cJSON_Delete(root);
        return json;
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}