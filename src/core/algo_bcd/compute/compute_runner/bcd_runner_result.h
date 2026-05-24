#ifndef BCD_RUNNER_RESULT_H
#define BCD_RUNNER_RESULT_H

#include "bcd_runner.h"

/**
 * Create an empty result struct with zero-initialized fields.
 */
static bcd_result_t *bcd_result_create(void);

/**
 * Transfer computed pipeline data into an already-created result struct.
 * Ownership of all passed resources moves to result.
 */
static void bcd_result_populate(bcd_result_t *result,
                                bcd_event_list_t event_list,
                                cvector_vector_type(bcd_cell_t) cell_list,
                                cvector_vector_type(int) path_list,
                                bcd_motion_plan_t motion_plan,
                                bool has_headland,
                                headland_t headland,
                                cvector_vector_type(point_t) start_nav,
                                vg_graph_t *vg);

/**
 * Frees all resources owned by result, then frees the struct itself.
 */
static void bcd_result_free(bcd_result_t *result);

#endif // BCD_RUNNER_RESULT_H
