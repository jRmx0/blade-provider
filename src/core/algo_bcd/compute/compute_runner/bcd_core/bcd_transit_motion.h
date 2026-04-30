#ifndef BCD_TRANSIT_MOTION_H
#define BCD_TRANSIT_MOTION_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_cell_computation.h"

/**
 * Generates an inter-cell navigation path connecting two coverage sections.
 * Walks the cell chain between begin_path_pos and end_path_pos in path_list,
 * inserting crossing waypoints at shared cell boundaries and medial-axis spine
 * waypoints at boundary kinks to keep the path inside concave cells.
 * Returns a vector of waypoints (nav), or NULL on error.
 * The caller is responsible for freeing the returned vector.
 */
cvector_vector_type(point_t) compute_connection_motion(const cvector_vector_type(bcd_cell_t) * cell_list,
                                                       const cvector_vector_type(int) * path_list,
                                                       int begin_path_pos,
                                                       point_t begin_point,
                                                       int end_path_pos,
                                                       point_t end_point);

#endif // BCD_TRANSIT_MOTION_H
