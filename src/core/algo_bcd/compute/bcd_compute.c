/**
 * bcd_compute.c
 *
 * Runs the BCD computation pipeline on a pre-validated, pre-parsed environment.
 * Contains no JSON or validation logic — those responsibilities live in bcd.c.
 *
 * Dependencies: internal.h, bcd_runner.c, bcd_core/ sources
 */

#include "../internal.h"
#include "../../../../dependencies/allocator/allocator.h"

/* Windows SDK defines IN/OUT as SAL annotation macros which collide with
 * the bcd_event_type_t enum values of the same name. */
#undef IN
#undef OUT

#include "compute_runner/bcd_core/bcd_event_list_building.c"
#include "compute_runner/bcd_core/bcd_cell_computation.c"
#include "compute_runner/bcd_core/bcd_pq.c"
#include "compute_runner/bcd_core/bcd_coverage_planning.c"
#include "compute_runner/bcd_core/bcd_geometry.c"
#include "compute_runner/bcd_core/bcd_pathfinding.c"
#include "compute_runner/bcd_core/bcd_ox_motion.c"
#include "compute_runner/bcd_core/bcd_motion_planning.c"
#include "../preprocess/bcd_preprocess.c"
#include "compute_runner/bcd_runner.c"

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