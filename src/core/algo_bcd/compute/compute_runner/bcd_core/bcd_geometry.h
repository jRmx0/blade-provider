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

#endif // BCD_GEOMETRY_H
