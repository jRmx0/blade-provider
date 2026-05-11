#include "../../../../../../dependencies/cvector/cvector.h"

#include "bcd_cell_computation.h"
#include "bcd_transit_motion.h"
#include "bcd_funnel.h"

// --- COMPUTE_CONNECTION_MOTION

static cvector_vector_type(int) extract_cell_chain(const cvector_vector_type(int) * path_list,
                                                   int begin_path_pos,
                                                   int end_path_pos);

// IMPLEMENTATION --- compute_connection_motion ----------------------

cvector_vector_type(point_t) compute_connection_motion(
    const cvector_vector_type(bcd_cell_t) * cell_list,
    const cvector_vector_type(int) * path_list,
    int begin_path_pos,
    point_t begin_point,
    int end_path_pos,
    point_t end_point)
{
    cvector_vector_type(point_t) nav = NULL;

    if (cell_list == NULL || *cell_list == NULL || path_list == NULL || *path_list == NULL)
        return nav;

    // Extract the ordered chain of cell indices by slicing path_list at the
    // exact known positions — never by searching for a value, because the same
    // cell index can appear multiple times (as an A* transit insertion and as a
    // primary coverage cell) and value-search would pick the wrong occurrence.
    cvector_vector_type(int) chain = extract_cell_chain(path_list, begin_path_pos, end_path_pos);

    if (chain == NULL)
        return nav;

    nav = bcd_funnel(cell_list, &chain, begin_point, end_point);

    cvector_free(chain);
    return nav;
}

// --- EXTRACT_CELL_CHAIN

static cvector_vector_type(int) extract_cell_chain(const cvector_vector_type(int) * path_list,
                                                   int begin_path_pos,
                                                   int end_path_pos)
{
    cvector_vector_type(int) chain = NULL;

    if (path_list == NULL)
        return chain;

    int path_size = (int)cvector_size(*path_list);
    if (begin_path_pos < 0 || begin_path_pos >= path_size ||
        end_path_pos < begin_path_pos || end_path_pos >= path_size)
        return chain;

    // Slice path_list directly by position — no value search needed.
    // This is safe even when the same cell index appears multiple times
    // in path_list (e.g. as both a BFS transit insertion and a primary
    // coverage cell), because we always have the exact positions.
    for (int i = begin_path_pos; i <= end_path_pos; ++i)
        cvector_push_back(chain, (*path_list)[i]);

    return chain;
}
