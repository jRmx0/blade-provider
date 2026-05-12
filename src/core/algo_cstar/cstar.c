#include "cstar.h"

#include "metadata/cstar_metadata.c"
#include "compute/cstar_compute.c"

char *cstar_get_metadata_json(void)
{
    return cstar_build_metadata_json();
}

char *cstar_compute(const char *input_environment_json)
{
    return cstar_run_compute(input_environment_json);
}