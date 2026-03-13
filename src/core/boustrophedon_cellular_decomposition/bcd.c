/**
 * bcd.c
 *
 * Main coordinator for the BCD algorithm module.
 * Implements the public interface declared in bcd.h.
 * Delegates metadata requests to bcd_metadata.c and
 * computation requests to compute/bcd_compute.c.
 * Contains no business logic of its own.
 *
 * Dependencies: bcd_metadata.c, compute/bcd_compute.c
 */

#include "bcd.h"
#include "internal.h"

char *bcd_get_metadata_json(void)
{
	return bcd_build_metadata_json();
}

char *bcd_compute(const char *input_environment_json)
{
	return bcd_run_compute(input_environment_json);
}