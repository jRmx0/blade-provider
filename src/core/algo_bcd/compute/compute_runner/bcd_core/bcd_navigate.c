#include "bcd_navigate.h"

#include "../../../../../../dependencies/cvector/cvector.h"
#include "bcd_pathfinding.h"
#include "bcd_funnel.h"

cvector_vector_type(point_t) bcd_navigate(
    const cvector_vector_type(bcd_cell_t) * cell_list,
    point_t from_point,
    point_t to_point)
{
    cvector_vector_type(point_t) result = NULL;

    if (cell_list == NULL || *cell_list == NULL)
        return result;

    int from_cell = bcd_find_cell(cell_list, from_point);
    int to_cell = bcd_find_cell(cell_list, to_point);

    if (from_cell < 0 || to_cell < 0)
        return result;

    cvector_vector_type(int) corridor = bcd_astar(from_cell, to_cell, cell_list);
    if (corridor == NULL)
        return result;

    result = bcd_funnel(cell_list, &corridor, from_point, to_point);

    cvector_free(corridor);
    return result;
}
