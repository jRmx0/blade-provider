#ifndef BCD_GEOMETRY_H
#define BCD_GEOMETRY_H

#include "bcd_cell_computation.h"

/**
 * Returns the interior point of a BCD cell guaranteed to lie inside the polygon.
 * Computes mid_x = (c_begin.x + c_end.x) / 2, interpolates both the ceiling
 * and floor piecewise-linear chains at that x, and returns their y midpoint.
 * Unlike the area centroid (shoelace), this is always strictly inside even for
 * highly concave floor/ceiling paths caused by obstacle deflection. For simple trapezoids this
 * reduces to the average of the four corner fields.
 */
point_t bcd_cell_interior_point(const bcd_cell_t *cell);

#endif // BCD_GEOMETRY_H
