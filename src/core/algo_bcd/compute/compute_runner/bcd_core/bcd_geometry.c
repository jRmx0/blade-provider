#include "bcd_geometry.h"

point_t bcd_cell_interior_point(const bcd_cell_t *cell)
{
    point_t p;
    p.x = (cell->c_begin.x + cell->c_end.x + cell->f_begin.x + cell->f_end.x) / 4.0f;
    p.y = (cell->c_begin.y + cell->c_end.y + cell->f_begin.y + cell->f_end.y) / 4.0f;
    return p;
}
