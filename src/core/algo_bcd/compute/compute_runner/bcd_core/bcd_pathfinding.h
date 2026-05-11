#ifndef BCD_PATHFINDING_H
#define BCD_PATHFINDING_H

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_cell_computation.h"

/**
 * A* search on the BCD cell adjacency graph.
 *
 * Returns a cvector(int) of cell indices representing the shortest path from
 * cell_index_from to cell_index_to (inclusive), or NULL if no path exists.
 * Heuristic: Euclidean distance between cell centroids.
 * The caller is responsible for freeing the returned vector with cvector_free().
 */
cvector_vector_type(int) bcd_astar(int cell_index_from,
                                   int cell_index_to,
                                   const cvector_vector_type(bcd_cell_t) * cell_list);

/**
 * Returns the index of the BCD cell that contains the given point, or -1 if
 * no cell contains it.
 *
 * Containment test: point.x is within [cell.c_begin.x, cell.c_end.x] AND
 * point.y is between the interpolated ceiling and floor of the cell at point.x.
 */
int bcd_find_cell(const cvector_vector_type(bcd_cell_t) * cell_list, point_t p);

#endif // BCD_PATHFINDING_H
