/**
 * bcd_compute.c
 *
 * Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
 * Contains no JSON or validation logic — those responsibilities live in bcd.c.
 *
 * Dependencies: internal.h, runner/bcd_runner.c
 */

#include "../bcd.h"
#include "../../../../dependencies/allocator/allocator.h"

/* Windows SDK defines IN/OUT as SAL annotation macros which collide with
 * the bcd_event_type_t enum values of the same name. */
#undef IN
#undef OUT

#include "runner/bcd_runner.c"

bcd_result_t *bcd_run_compute(input_environment_t *environment)
{
	if (environment == NULL)
	{
		return NULL;
	}

	bcd_compute_error_t compute_err = {NULL, NULL};
	bcd_result_t *result = coverage_path_planning_process(environment, &compute_err);
	return result;
}