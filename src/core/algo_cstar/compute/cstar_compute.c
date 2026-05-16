#include "../../../../dependencies/cJSON/cJSON.h"
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

static char *cstar_create_not_implemented_json(void)
{
    cJSON *response = cJSON_CreateObject();
    if (response == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "code", "not_implemented");
    cJSON_AddStringToObject(response, "message", "C* compute pipeline is not implemented yet.");

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

    cstar_check_free_environment(&environment);
    return cstar_create_not_implemented_json();
}