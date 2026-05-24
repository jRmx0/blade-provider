#ifndef BCD_OX_MOTION_H
#define BCD_OX_MOTION_H

#include "../../../../../../../dependencies/cvector/cvector.h"
#include "../cells/bcd_cell_computation.h"

/**
 * Generates a boustrophedon (back-and-forth) sweep path for a single BCD cell.
 * Returns a vector of waypoints forming the continuous coverage path (ox),
 * or NULL on error. The caller is responsible for freeing the returned vector.
 */
cvector_vector_type(point_t) compute_boustrophedon_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                          int cell_index,
                                                          float step_size);

#endif // BCD_OX_MOTION_H
