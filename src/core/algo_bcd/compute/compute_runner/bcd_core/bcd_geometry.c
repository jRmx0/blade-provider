#include "bcd_geometry.h"

/* ---- grid resolution and refinement tolerance ---- */
#define FARTHEST_GRID_N 32
#define FARTHEST_MIN_STEP 1e-4f

/* ------------------------------------------------------------------ */
/* Static helpers                                                       */
/* ------------------------------------------------------------------ */

/*
 * Squared distance from point P to segment AB.
 * Uses clamped projection – avoids sqrtf and valid for comparisons.
 */
static float seg_dist_sq(point_t P, point_t A, point_t B)
{
    float abx = B.x - A.x, aby = B.y - A.y;
    float apx = P.x - A.x, apy = P.y - A.y;
    float len_sq = abx * abx + aby * aby;
    if (len_sq < 1e-12f)
    {
        /* degenerate segment – treat as point */
        return apx * apx + apy * apy;
    }
    float t = (apx * abx + apy * aby) / len_sq;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    float dx = apx - t * abx;
    float dy = apy - t * aby;
    return dx * dx + dy * dy;
}

/*
 * Interpolate ceiling y and floor y at a given x inside the cell.
 *
 * Ceiling chain (left → right):  c_begin → ceiling_edge_list[*].end → c_end
 * Floor chain   (right → left):  f_begin → floor_edge_list[*].end (reversed) → f_end
 */
static void cell_interp_bounds(const bcd_cell_t *cell, float x,
                               float *out_ceil_y, float *out_floor_y)
{
    int ceil_n = (int)cvector_size(cell->ceiling_edge_list);
    int floor_n = (int)cvector_size(cell->floor_edge_list);

    /* ceiling y (left → right) */
    float ceil_y = cell->c_begin.y;
    {
        float ax = cell->c_begin.x, ay = cell->c_begin.y;
        for (int j = 0; j < ceil_n; ++j)
        {
            float bx = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.x
                                        : cell->c_end.x;
            float by = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.y
                                        : cell->c_end.y;
            if (x <= bx || j == ceil_n - 1)
            {
                float dx = bx - ax;
                ceil_y = (dx < 1e-9f) ? ay : ay + (x - ax) / dx * (by - ay);
                break;
            }
            ax = bx;
            ay = by;
        }
    }

    /* floor y (right → left) */
    float floor_y = cell->f_begin.y;
    {
        float ax = cell->f_begin.x, ay = cell->f_begin.y;
        for (int j = floor_n - 1; j >= 0; --j)
        {
            float bx = (j > 0) ? cell->floor_edge_list[j].end.x : cell->f_end.x;
            float by = (j > 0) ? cell->floor_edge_list[j].end.y : cell->f_end.y;
            if (x >= bx || j == 0)
            {
                float dx = ax - bx;
                floor_y = (dx < 1e-9f) ? ay : ay + (ax - x) / dx * (by - ay);
                break;
            }
            ax = bx;
            ay = by;
        }
    }

    *out_ceil_y = ceil_y;
    *out_floor_y = floor_y;
}

/*
 * Minimum squared distance from point P to the full boundary of the cell.
 *
 * Boundary segments (one closed polygon, walking counter-clockwise):
 *   Left vertical:  c_begin  → f_end
 *   Ceiling chain:  c_begin  → [ceiling_edge_list vertices] → c_end
 *   Right vertical: c_end    → f_begin
 *   Floor chain:    f_begin  → [floor_edge_list vertices reversed] → f_end
 */
static float cell_min_edge_dist_sq(point_t P, const bcd_cell_t *cell)
{
    int ceil_n = (int)cvector_size(cell->ceiling_edge_list);
    int floor_n = (int)cvector_size(cell->floor_edge_list);
    float min_d = 1e38f;
    float d;

    /* left vertical */
    d = seg_dist_sq(P, cell->c_begin, cell->f_end);
    if (d < min_d)
        min_d = d;

    /* ceiling chain (left → right) */
    if (ceil_n == 0)
    {
        d = seg_dist_sq(P, cell->c_begin, cell->c_end);
        if (d < min_d)
            min_d = d;
    }
    else
    {
        float ax = cell->c_begin.x, ay = cell->c_begin.y;
        for (int j = 0; j < ceil_n; ++j)
        {
            float bx = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.x
                                        : cell->c_end.x;
            float by = (j < ceil_n - 1) ? cell->ceiling_edge_list[j].end.y
                                        : cell->c_end.y;
            point_t A = {ax, ay}, B = {bx, by};
            d = seg_dist_sq(P, A, B);
            if (d < min_d)
                min_d = d;
            ax = bx;
            ay = by;
        }
    }

    /* right vertical */
    d = seg_dist_sq(P, cell->c_end, cell->f_begin);
    if (d < min_d)
        min_d = d;

    /* floor chain (right → left) */
    if (floor_n == 0)
    {
        d = seg_dist_sq(P, cell->f_begin, cell->f_end);
        if (d < min_d)
            min_d = d;
    }
    else
    {
        float ax = cell->f_begin.x, ay = cell->f_begin.y;
        for (int j = floor_n - 1; j >= 0; --j)
        {
            float bx = (j > 0) ? cell->floor_edge_list[j].end.x : cell->f_end.x;
            float by = (j > 0) ? cell->floor_edge_list[j].end.y : cell->f_end.y;
            point_t A = {ax, ay}, B = {bx, by};
            d = seg_dist_sq(P, A, B);
            if (d < min_d)
                min_d = d;
            ax = bx;
            ay = by;
        }
    }

    return min_d;
}

/* ------------------------------------------------------------------ */

point_t bcd_cell_farthest_interior_point(const bcd_cell_t *cell)
{
    /*
     * Returns the pole of inaccessibility of the BCD cell: the interior point
     * whose minimum distance to any boundary segment is maximised.
     *
     * Works on non-convex cells (concave ceiling/floor from obstacle deflection).
     *
     * Two-phase algorithm:
     *
     * Phase 1 – FARTHEST_GRID_N × FARTHEST_GRID_N interior grid scan.
     *   For each column x, interpolate ceiling/floor with cell_interp_bounds,
     *   then uniformly sample y in [ceil_y, floor_y].  Every candidate is
     *   guaranteed to lie strictly inside.  Track the point with the largest
     *   minimum-boundary squared-distance.
     *
     * Phase 2 – Coordinate-descent refinement starting from the grid winner.
     *   Try ±step in x and y; accept any improving move; halve step when stuck.
     *   Terminate when step < FARTHEST_MIN_STEP.  After each accepted x-move the
     *   new y is clamped to the interior at that column.  Because boundary-clamped
     *   candidates have min-dist 0 they are never accepted, so clamping is safe.
     */
    float left_x = cell->c_begin.x;
    float right_x = cell->c_end.x;
    float cell_width = right_x - left_x;

    point_t best = cell->c_begin; /* overwritten by first valid grid hit */
    float best_d = -1.0f;

    /* ---- Phase 1: grid scan ---- */
    for (int ix = 0; ix < FARTHEST_GRID_N; ++ix)
    {
        float x = left_x + (ix + 0.5f) / (float)FARTHEST_GRID_N * cell_width;
        float ceil_y, floor_y;
        cell_interp_bounds(cell, x, &ceil_y, &floor_y);

        float h = floor_y - ceil_y;
        if (h < 1e-9f)
            continue; /* degenerate column, skip */

        for (int iy = 0; iy < FARTHEST_GRID_N; ++iy)
        {
            point_t P;
            P.x = x;
            P.y = ceil_y + (iy + 0.5f) / (float)FARTHEST_GRID_N * h;

            float d = cell_min_edge_dist_sq(P, cell);
            if (d > best_d)
            {
                best_d = d;
                best = P;
            }
        }
    }

    /* ---- Phase 2: coordinate-descent refinement ---- */
    float step = cell_width / (float)FARTHEST_GRID_N;
    while (step > FARTHEST_MIN_STEP)
    {
        int improved = 0;

        const float ddx[4] = {step, -step, 0.0f, 0.0f};
        const float ddy[4] = {0.0f, 0.0f, step, -step};

        for (int k = 0; k < 4; ++k)
        {
            float nx = best.x + ddx[k];
            float ny = best.y + ddy[k];

            /* clamp x to cell x-range */
            if (nx < left_x)
                nx = left_x;
            if (nx > right_x)
                nx = right_x;

            /* clamp y to interior at the new x */
            float cy, fy;
            cell_interp_bounds(cell, nx, &cy, &fy);
            if (fy - cy < 1e-9f)
                continue;
            if (ny < cy)
                ny = cy;
            if (ny > fy)
                ny = fy;

            point_t P;
            P.x = nx;
            P.y = ny;

            float d = cell_min_edge_dist_sq(P, cell);
            if (d > best_d)
            {
                best_d = d;
                best = P;
                improved = 1;
            }
        }

        if (!improved)
            step *= 0.5f;
    }

    return best;
}

point_t bcd_cell_midpoint_at_x(const bcd_cell_t *cell, float x)
{
    float ceil_y, floor_y;
    cell_interp_bounds(cell, x, &ceil_y, &floor_y);
    point_t p;
    p.x = x;
    p.y = (ceil_y + floor_y) / 2.0f;
    return p;
}

int bcd_find_starting_cell(const cvector_vector_type(bcd_cell_t) * cell_list, point_t p)
{
    if (cell_list == NULL || cvector_size(*cell_list) == 0)
        return 0;

    int cell_count = (int)cvector_size(*cell_list);

    /* Phase 1: containment check — return the cell that contains p. */
    for (int i = 0; i < cell_count; ++i)
    {
        const bcd_cell_t *cell = &(*cell_list)[i];
        if (p.x < cell->c_begin.x || p.x > cell->c_end.x)
            continue;

        float ceil_y, floor_y;
        cell_interp_bounds(cell, p.x, &ceil_y, &floor_y);

        if (p.y >= ceil_y && p.y <= floor_y)
            return i;
    }

    /* Phase 2: fallback — nearest cell by x-midpoint distance. */
    int best_index = 0;
    float best_dist = 1e38f;
    for (int i = 0; i < cell_count; ++i)
    {
        const bcd_cell_t *cell = &(*cell_list)[i];
        float mid_x = (cell->c_begin.x + cell->c_end.x) / 2.0f;
        float dist = p.x - mid_x;
        if (dist < 0.0f)
            dist = -dist;
        if (dist < best_dist)
        {
            best_dist = dist;
            best_index = i;
        }
    }
    return best_index;
}
