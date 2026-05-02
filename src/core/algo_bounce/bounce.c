#include "bounce.h"
#include "internal.h"

#include "check/bounce_check.h"
#include "metadata/bounce_metadata.c"
#include "compute/bounce_compute.c"

char *bounce_get_metadata_json(void)
{
    return bounce_build_metadata_json();
}

char *bounce_compute(const char *input_environment_json)
{
    return bounce_run_compute(input_environment_json);
}