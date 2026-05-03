/**
 * bounce_compute.c
 *
 * Validates request JSON and delegates to the Bounce pipeline runner.
 * Error mapping and environment cleanup are centralized here.
 *
 * Unity-build includes — processed in dependency order:
 *   step modules (leaf, no inter-step dependencies)
 *   compute_runner/bounce_runner.c (depends on step headers)
 *
 * Dependencies: internal.h, check/bounce_check.h, compute_runner/bounce_runner.h
 */

#include "../internal.h"
#include "../check/bounce_check.h"
#include "../../../../dependencies/cJSON/cJSON.h"

#include "steps/bounce_headland_step.c"
#include "steps/bounce_angle_step.c"
#include "steps/bounce_ray_step.c"
#include "steps/bounce_metrics_step.c"
#include "compute_runner/bounce_runner.c"

static char *bounce_create_error_json(const char *code, const char *message)
{
    const char *safe_code = (code != NULL && code[0] != '\0') ? code : "bounce_error";
    const char *safe_message = (message != NULL && message[0] != '\0') ? message : "Bounce compute failed.";

    cJSON *response = cJSON_CreateObject();
    if (response == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "code", safe_code);
    cJSON_AddStringToObject(response, "message", safe_message);

    char *json = cJSON_PrintUnformatted(response);
    cJSON_Delete(response);
    return json;
}

char *bounce_run_compute(const char *input_environment_json)
{
    input_environment_t environment;
    bounce_check_result_t check_result = {.ok = true, .code = NULL, .message = NULL};

    if (!bounce_check_request_json(input_environment_json, &environment, &check_result))
    {
        return bounce_create_error_json(check_result.code, check_result.message);
    }

    cJSON *result = bounce_run_pipeline(&environment);

    bounce_free_input_environment(&environment);

    if (result == NULL)
    {
        return bounce_create_error_json("allocation_failed", "Bounce pipeline allocation failed.");
    }

    char *json = cJSON_PrintUnformatted(result);
    cJSON_Delete(result);
    return json;
}
