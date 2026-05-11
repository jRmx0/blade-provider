#ifndef BCD_FUNNEL_H
#define BCD_FUNNEL_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_cell_computation.h"

/**
 * Funnel algorithm (Simple Stupid Funnel Algorithm) over a BCD cell corridor.
 *
 * Given an ordered list of cell indices (cell_corridor, length >= 1) describing
 * the cells to pass through, and start/end points within those cells, produces
 * the geometrically shortest piecewise-linear path that stays inside the
 * union of those cells.
 *
 * Returns a cvector(point_t) including from_point and to_point, or NULL on error.
 * The caller is responsible for freeing the returned vector with cvector_free().
 *
 * cell_corridor must be a valid contiguous chain of adjacent BCD cells.
 */
cvector_vector_type(point_t) bcd_funnel(
    const cvector_vector_type(bcd_cell_t) * cell_list,
    const cvector_vector_type(int) * cell_corridor,
    point_t from_point,
    point_t to_point);

#endif // BCD_FUNNEL_H
