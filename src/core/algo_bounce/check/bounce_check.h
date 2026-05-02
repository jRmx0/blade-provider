#ifndef BOUNCE_CHECK_H
#define BOUNCE_CHECK_H

#include "../internal.h"

typedef struct
{
    bool ok;
    const char *code;
    const char *message;
} bounce_check_result_t;

bool bounce_check_request_json(const char *request_json, input_environment_t *environment, bounce_check_result_t *result);

static inline void bounce_free_input_environment(input_environment_t *env)
{
    if (env == NULL)
    {
        return;
    }
    free_polygon(&env->boundary);
    if (env->obstacles != NULL)
    {
        for (uint32_t i = 0; i < env->obstacle_count; i++)
        {
            free_polygon(&env->obstacles[i]);
        }
        free(env->obstacles);
        env->obstacles = NULL;
    }
    env->obstacle_count = 0;
}

#endif // BOUNCE_CHECK_H
