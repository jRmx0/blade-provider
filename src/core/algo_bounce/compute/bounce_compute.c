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

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "steps/bounce_headland_step.c"
#include "steps/bounce_angle_step.c"
#include "steps/bounce_ray_step.c"
#include "steps/bounce_metrics_step.c"
#include "compute_runner/bounce_runner.c"

static uint32_t bounce_hash_seed_string(const char *seed_text)
{
    // FNV-1a 32-bit hash
    uint32_t hash = 2166136261u;
    if (seed_text == NULL)
    {
        return hash;
    }

    while (*seed_text != '\0')
    {
        hash ^= (uint8_t)*seed_text;
        hash *= 16777619u;
        seed_text++;
    }

    return (hash == 0u) ? 1u : hash;
}

static uint32_t bounce_entropy_seed(void)
{
    uint64_t t = (uint64_t)time(NULL);
    uint64_t c = (uint64_t)clock();
    uintptr_t addr = (uintptr_t)&t;

    uint64_t mix = t ^ (c << 21) ^ (addr >> 3);
    uint32_t seed = (uint32_t)(mix ^ (mix >> 32));

    return (seed == 0u) ? 0xA341316Cu : seed;
}

static void bounce_init_random_seed(const char *input_environment_json)
{
    uint32_t seed_value = bounce_entropy_seed();

    cJSON *root = cJSON_Parse(input_environment_json);
    if (root != NULL)
    {
        cJSON *parameters = cJSON_GetObjectItemCaseSensitive(root, "parameters");
        if (parameters != NULL && cJSON_IsObject(parameters))
        {
            cJSON *seed_item = cJSON_GetObjectItemCaseSensitive(parameters, "Seed");
            if (seed_item == NULL)
            {
                seed_item = cJSON_GetObjectItemCaseSensitive(parameters, "seed");
            }

            if (seed_item != NULL && cJSON_IsString(seed_item) &&
                seed_item->valuestring != NULL && seed_item->valuestring[0] != '\0')
            {
                seed_value = bounce_hash_seed_string(seed_item->valuestring);
            }
        }

        cJSON_Delete(root);
    }

    srand(seed_value);
}

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

    // Seed all Bounce random operations for this request.
    // - Non-empty parameters.Seed -> deterministic run.
    // - Missing/empty Seed        -> entropy-seeded run (different each time).
    bounce_init_random_seed(input_environment_json);

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
