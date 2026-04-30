#ifndef BCD_GEOMETRY_H
#define BCD_GEOMETRY_H

#include "bcd_cell_computation.h"

/**
 * Returns the pole of inaccessibility of the BCD cell: the interior point
 * that maximises the minimum distance to any boundary segment (ceiling, floor,
 * left vertical, right vertical). Equivalent to the centre of the largest
 * inscribed circle that fits inside the cell.
 *
 * Works correctly on non-convex cells produced by obstacle deflection.
 * Uses a 32×32 interior grid scan followed by coordinate-descent refinement.
 * The returned point is always strictly inside the cell.
 */
point_t bcd_cell_farthest_interior_point(const bcd_cell_t *cell);

/**
 * Returns the midpoint between the ceiling and floor of the cell at a given x.
 * Uses the piecewise-linear boundary chains, so the result correctly tracks
 * concave (obstacle-deflected) boundaries.
 * x must lie within [c_begin.x, c_end.x]; no clamping is performed.
 */
point_t bcd_cell_midpoint_at_x(const bcd_cell_t *cell, float x);

/**
 * Returns the index of the cell in cell_list that contains point p, or the
 * index of the cell whose x-midpoint is closest to p.x when p lies outside
 * all cells. Returns 0 when cell_list is empty.
 */
int bcd_find_starting_cell(const cvector_vector_type(bcd_cell_t) * cell_list, point_t p);

#endif // BCD_GEOMETRY_H
