#ifndef BCD_PATHFINDING_H
#define BCD_PATHFINDING_H

#include "../../../../../../../dependencies/cvector/cvector.h"
#include "../cells/bcd_cell_computation.h"

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

#endif // BCD_PATHFINDING_H
