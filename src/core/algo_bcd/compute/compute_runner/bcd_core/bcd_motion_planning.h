#ifndef BCD_MOTION_H
#define BCD_MOTION_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_cell_computation.h"

typedef struct
{
    cvector_vector_type(point_t) ox;
    cvector_vector_type(point_t) nav; // transit path to next section/end (assigned in bcd_runner.c)
    int path_list_index;              // index into path_list where this section was generated
} cell_motion_plan_t;

typedef struct
{
    cvector_vector_type(cell_motion_plan_t) section;
} bcd_motion_plan_t;

int compute_bcd_motion(cvector_vector_type(bcd_cell_t) * cell_list,
                       const cvector_vector_type(int) * path_list,
                       bcd_motion_plan_t *motion_plan,
                       float step_size,
                       point_t end_point);

void log_bcd_motion(const bcd_motion_plan_t motion_plan);

void free_bcd_motion(bcd_motion_plan_t *motion_plan);

#endif // BCD_MOTION_H