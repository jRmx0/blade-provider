#include "bcd_geometry.h"

/* Add a point to the running sum only if it differs from the last added point.
 * Deduplicates consecutive equal vertices that arise in triangle (SIDE_IN/OUT)
 * cells where c_begin==f_end and c_end==f_begin. */
static void accumulate(float x, float y,
                       float *prev_x, float *prev_y,
                       float *sum_x, float *sum_y, int *count)
{
    if (*count > 0 && x == *prev_x && y == *prev_y)
        return;
    *sum_x += x;
    *sum_y += y;
    ++(*count);
    *prev_x = x;
    *prev_y = y;
}

point_t bcd_cell_interior_point(const bcd_cell_t *cell)
{
    float sum_x = 0.0f, sum_y = 0.0f;
    float prev_x = 0.0f, prev_y = 0.0f;
    int count = 0;

    int ceil_n  = (int)cvector_size(cell->ceiling_edge_list);
    int floor_n = (int)cvector_size(cell->floor_edge_list);

    accumulate(cell->c_begin.x, cell->c_begin.y, &prev_x, &prev_y, &sum_x, &sum_y, &count);
    for (int i = 0; i < ceil_n - 1; ++i)
        accumulate(cell->ceiling_edge_list[i].end.x, cell->ceiling_edge_list[i].end.y,
                   &prev_x, &prev_y, &sum_x, &sum_y, &count);
    accumulate(cell->c_end.x,   cell->c_end.y,   &prev_x, &prev_y, &sum_x, &sum_y, &count);
    accumulate(cell->f_begin.x, cell->f_begin.y, &prev_x, &prev_y, &sum_x, &sum_y, &count);
    for (int i = 0; i < floor_n - 1; ++i)
        accumulate(cell->floor_edge_list[i].end.x, cell->floor_edge_list[i].end.y,
                   &prev_x, &prev_y, &sum_x, &sum_y, &count);
    /* f_end: also check wrap-around against c_begin */
    if (cell->f_end.x != cell->c_begin.x || cell->f_end.y != cell->c_begin.y)
        accumulate(cell->f_end.x, cell->f_end.y, &prev_x, &prev_y, &sum_x, &sum_y, &count);

    point_t p;
    p.x = (count > 0) ? sum_x / (float)count : 0.0f;
    p.y = (count > 0) ? sum_y / (float)count : 0.0f;
    return p;
}
