#include "../../../../dependencies/cJSON/cJSON.h"
#include "../internal.h"
#include "runner/cstar_runner.h"

#include "runner/cstar_runner.c"

cstar_coverage_path_result_t *cstar_run_compute(cstar_environment_t *environment)
{
    if (environment == NULL)
    {
        return NULL;
    }

    cstar_coverage_path_result_t *result = cstar_coverage_path_planning_process(environment);
    return result;
}