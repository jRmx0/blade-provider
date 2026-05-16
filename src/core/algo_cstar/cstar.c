#include "cstar.h"
#include "../../../dependencies/cJSON/cJSON.h"
#include "check/cstar_check.h"
#include "parser/cstar_parser.h"

#include "check/cstar_check.c"
#include "parser/cstar_parser.c"
#include "metadata/cstar_metadata.c"
#include "compute/cstar_compute.c"

static char *cstar_create_entrypoint_error_json(const char *code, const char *message)
{
    cJSON *response = cJSON_CreateObject();
    if (response == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "code", code != NULL ? code : "cstar_error");
    cJSON_AddStringToObject(response, "message", message != NULL ? message : "C* compute pipeline failed.");

    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    return json;
}

char *cstar_get_metadata_json(void)
{
    return cstar_build_metadata_json();
}

char *cstar_compute(const char *input_environment_json)
{
    cstar_environment_t environment;
    cstar_check_result_t check_result;

    if (!cstar_validate_request_json(input_environment_json, &check_result))
    {
        return cstar_create_entrypoint_error_json(check_result.code, check_result.message);
    }

    if (!cstar_parse_request_json(input_environment_json, &environment, &check_result))
    {
        return cstar_create_entrypoint_error_json(check_result.code, check_result.message);
    }

    char *json = cstar_run_compute(&environment);
    cstar_parser_free_environment(&environment);

    if (json == NULL)
    {
        return cstar_create_entrypoint_error_json("allocation_failed", "C* compute pipeline allocation failed.");
    }

    return json;
}