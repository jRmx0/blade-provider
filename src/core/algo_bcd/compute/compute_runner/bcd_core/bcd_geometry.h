#ifndef BCD_GEOMETRY_H
#define BCD_GEOMETRY_H

#include "bcd_cell_computation.h"

/**
 * Returns the centroid of a BCD cell. Computed as the average of all polygon
 * vertices: c_begin, ceiling intermediate deflection vertices, c_end, f_begin,
 * floor intermediate deflection vertices, f_end. For simple trapezoids this
 * reduces to the average of the four corner fields.
 */
point_t bcd_cell_interior_point(const bcd_cell_t *cell);

#endif // BCD_GEOMETRY_H
