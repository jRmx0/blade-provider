#include "bcd_geometry.h"

point_t bcd_cell_interior_point(const bcd_cell_t *cell)
{
    /*
     * Returns the horizontal midpoint of the cell with the y midpoint of the
     * ceiling and floor boundaries evaluated at that x.  This is always
     * strictly inside the cell regardless of floor/ceiling concavity.
     *
     * Ceiling piecewise-linear chain (left → right):
     *   c_begin → ceiling_edge_list[0..ceil_n-2].end → c_end
     *
     * Floor piecewise-linear chain (right → left):
     *   f_begin → floor_edge_list[floor_n-1..1].end → f_end
     */
    float mid_x = (cell->c_begin.x + cell->c_end.x) * 0.5f;

    /* ---- ceiling y at mid_x (left → right) ---- */
    int   ceil_n = (int)cvector_size(cell->ceiling_edge_list);
    float ceil_y = cell->c_begin.y;
    {
        float ax = cell->c_begin.x, ay = cell->c_begin.y;
        for (int j = 0; j < ceil_n; ++j)
        {
            float bx = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.x
                                         : cell->c_end.x;
            float by = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.y
                                         : cell->c_end.y;
            if (mid_x <= bx || j == ceil_n - 1)
            {
                float dx = bx - ax;
                ceil_y = (dx < 1e-9f) ? ay
                                       : ay + (mid_x - ax) / dx * (by - ay);
                break;
            }
            ax = bx; ay = by;
        }
    }

    /* ---- floor y at mid_x (right → left) ---- */
    int   floor_n = (int)cvector_size(cell->floor_edge_list);
    float floor_y = cell->f_begin.y;
    {
        float ax = cell->f_begin.x, ay = cell->f_begin.y;
        for (int j = floor_n - 1; j >= 0; --j)
        {
            float bx = (j > 0) ? cell->floor_edge_list[j].end.x : cell->f_end.x;
            float by = (j > 0) ? cell->floor_edge_list[j].end.y : cell->f_end.y;
            if (mid_x >= bx || j == 0)
            {
                float dx = ax - bx;
                floor_y = (dx < 1e-9f) ? ay
                                        : ay + (ax - mid_x) / dx * (by - ay);
                break;
            }
            ax = bx; ay = by;
        }
    }

    point_t p;
    p.x = mid_x;
    p.y = (ceil_y + floor_y) * 0.5f;
    return p;
}
