
#include "cstar.h"
#include "check/cstar_check.h"
#include "parser/cstar_parser.h"
#include "serialize/cstar_serialize.h"

#include "check/cstar_check.c"
#include "parser/cstar_parser.c"
#include "metadata/cstar_metadata.c"
#include "compute/cstar_compute.c"
#include "serialize/cstar_serialize.c"

// Entrypoint error serialization now handled by serialize module

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
        return cstar_serialize_error_json(check_result.code, check_result.message);
    }

    if (!cstar_parse_request_json(input_environment_json, &environment, &check_result))
    {
        return cstar_serialize_error_json(check_result.code, check_result.message);
    }

    cstar_coverage_path_result_t *result = cstar_run_compute(&environment);
    cstar_parser_free_environment(&environment);

    if (result == NULL)
    {
        return cstar_serialize_error_json("allocation_failed", "C* compute pipeline allocation failed.");
    }

    char *json = cstar_serialize_result_json(result);
    cstar_result_free(result);

    return json;
}