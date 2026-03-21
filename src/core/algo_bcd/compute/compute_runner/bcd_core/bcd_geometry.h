#ifndef BCD_GEOMETRY_H
#define BCD_GEOMETRY_H

#include "bcd_cell_computation.h"

/**
 * Returns the interior point of a BCD cell that is furthest from all cell
 * boundaries. Computed as the centroid of the four span corners
 * (c_begin, c_end, f_begin, f_end), which is an accurate approximation of the
 * Chebyshev center for the trapezoid shapes BCD produces.
 */
point_t bcd_cell_interior_point(const bcd_cell_t *cell);

#endif // BCD_GEOMETRY_H
