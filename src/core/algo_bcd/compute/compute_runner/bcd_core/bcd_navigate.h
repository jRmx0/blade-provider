#ifndef BCD_NAVIGATE_H
#define BCD_NAVIGATE_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_cell_computation.h"

/**
 * Computes the shortest path between two arbitrary points within the BCD cell
 * decomposition, without leaving the covered area.
 *
 * Pipeline:
 *   1. bcd_find_cell(from_point) and bcd_find_cell(to_point) — locate cells.
 *   2. bcd_astar(from_cell, to_cell) — find the minimal-cost cell corridor.
 *   3. bcd_funnel(corridor, from_point, to_point) — extract shortest geometric path.
 *
 * Returns a cvector(point_t) including from_point and to_point, or NULL if:
 *   - Either point is outside all cells.
 *   - No path exists between the two cells (disconnected graph).
 *
 * The caller is responsible for freeing the returned vector with cvector_free().
 */
cvector_vector_type(point_t) bcd_navigate(
    const cvector_vector_type(bcd_cell_t) * cell_list,
    point_t from_point,
    point_t to_point);

#endif // BCD_NAVIGATE_H
