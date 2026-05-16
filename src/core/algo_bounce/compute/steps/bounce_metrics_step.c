/**
 * bounce_metrics_step.c
 *
 * Step 4: Updates cumulative path metrics after a segment is committed.
 *
 * Coverage ratio method — grid-based capsule marking:
 *   1. On the first call an AOI cell grid is built over the realworld bounding
 *      box when available, otherwise the transformed environment bounding box.
 *      Each cell is valid if its centre lies inside the selected boundary and
 *      outside the selected obstacles.
 *   2. Per segment: every valid cell whose centre is within path_width/2 of
 *      the segment line (capsule footprint) is marked covered. A cell is
 *      counted at most once regardless of how many segments pass over it.
 *   3. estimated_coverage = (covered_cells / valid_cells) * 100.
 *
 * Cell size = parameter "Coverage Grid Cell Size" when > 0, otherwise
 * path_width / 6. The final size is auto-scaled to stay within a 1 M cell
 * budget so large fields remain fast while small fields stay precise.
 *
 * Dependencies: bounce_metrics_step.h
 */

#include "bounce_metrics_step.h"

#include <math.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Local geometry helpers
// ---------------------------------------------------------------------------

/**
 * Ray-casting point-in-polygon test using raw vertex array.
 * Returns true when p lies strictly inside the polygon.
 */
static bool bounce_metrics_point_in_polygon(point_t p,
                                            const point_t *verts,
                                            uint32_t n)
{
    bool inside = false;
    for (uint32_t i = 0, j = n - 1; i < n; j = i++)
    {
        float xi = verts[i].x, yi = verts[i].y;
        float xj = verts[j].x, yj = verts[j].y;
        if (((yi > p.y) != (yj > p.y)) &&
            (p.x < (xj - xi) * (p.y - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

/**
 * Minimum distance from point p to segment (a, b).
 */
static float bounce_metrics_point_to_segment_dist(point_t p,
                                                  point_t a,
                                                  point_t b)
{
    float abx = b.x - a.x, aby = b.y - a.y;
    float apx = p.x - a.x, apy = p.y - a.y;
    float ab2 = abx * abx + aby * aby;
    if (ab2 < 1e-12f)
        return sqrtf(apx * apx + apy * apy);
    float t = (apx * abx + apy * aby) / ab2;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    float dx = apx - t * abx;
    float dy = apy - t * aby;
    return sqrtf(dx * dx + dy * dy);
}

static float bounce_metrics_clampf(float value, float min_v, float max_v)
{
    if (value < min_v)
        return min_v;
    if (value > max_v)
        return max_v;
    return value;
}

// ---------------------------------------------------------------------------
// Coverage grid — build / mark / free
// ---------------------------------------------------------------------------

/**
 * Builds the AOI grid from the original environment.
 * Must be called once before marking any segments.
 */
static void bounce_coverage_grid_build(bounce_coverage_grid_t *grid,
                                       const input_environment_t *env)
{
    memset(grid, 0, sizeof(bounce_coverage_grid_t));

    if (env == NULL)
        return;

    const polygon_t *boundary = &env->boundary;
    const polygon_t *obstacles = env->obstacles;
    uint32_t obstacle_count = env->obstacle_count;

    if (env->realworld_boundary.vertices != NULL && env->realworld_boundary.vertex_count >= 3)
        boundary = &env->realworld_boundary;

    if (env->realworld_obstacles != NULL && env->realworld_obstacle_count > 0)
    {
        obstacles = env->realworld_obstacles;
        obstacle_count = env->realworld_obstacle_count;
    }

    if (boundary->vertices == NULL || boundary->vertex_count < 3)
        return;

    // Boundary bounding box.
    float min_x = boundary->vertices[0].x;
    float max_x = min_x;
    float min_y = boundary->vertices[0].y;
    float max_y = min_y;
    for (uint32_t i = 1; i < boundary->vertex_count; ++i)
    {
        float x = boundary->vertices[i].x;
        float y = boundary->vertices[i].y;
        if (x < min_x)
            min_x = x;
        if (x > max_x)
            max_x = x;
        if (y < min_y)
            min_y = y;
        if (y > max_y)
            max_y = y;
    }

    float field_w = max_x - min_x;
    float field_h = max_y - min_y;
    if (field_w < 1e-3f || field_h < 1e-3f)
        return;

    // Cell size = parameter override when > 0, else path_width / 6,
    // then bumped up as needed to stay under 1 M cells.
    float path_width = (env->path_width > 0.1f) ? env->path_width : 1.0f;
    float cell_size = (env->coverage_grid_cell_size > 0.0f)
                          ? env->coverage_grid_cell_size
                          : (path_width / 6.0f);

    const float MAX_CELLS = 1000000.0f;
    float min_cs = sqrtf(field_w * field_h / MAX_CELLS);
    if (cell_size < min_cs)
        cell_size = min_cs;

    grid->origin_x = min_x;
    grid->origin_y = min_y;
    grid->cell_size = cell_size;
    grid->cols = (uint32_t)(field_w / cell_size) + 2;
    grid->rows = (uint32_t)(field_h / cell_size) + 2;

    uint32_t total = grid->cols * grid->rows;
    if (total == 0)
        return;

    grid->cells = (uint8_t *)va_calloc((size_t)total, sizeof(uint8_t));
    if (grid->cells == NULL)
        return;

    // Mark valid cells: inside boundary AND outside all obstacles.
    for (uint32_t row = 0; row < grid->rows; ++row)
    {
        for (uint32_t col = 0; col < grid->cols; ++col)
        {
            point_t center = {
                grid->origin_x + ((float)col + 0.5f) * grid->cell_size,
                grid->origin_y + ((float)row + 0.5f) * grid->cell_size};

            if (!bounce_metrics_point_in_polygon(center,
                                                 boundary->vertices,
                                                 boundary->vertex_count))
                continue;

            bool blocked = false;
            for (uint32_t k = 0; !blocked && k < obstacle_count; ++k)
            {
                if (obstacles[k].vertices != NULL &&
                    obstacles[k].vertex_count >= 3 &&
                    bounce_metrics_point_in_polygon(center,
                                                    obstacles[k].vertices,
                                                    obstacles[k].vertex_count))
                    blocked = true;
            }
            if (blocked)
                continue;

            grid->cells[row * grid->cols + col] = 0x01; // valid
            ++grid->valid_count;
        }
    }
}

/**
 * Marks all valid AOI cells covered by the capsule footprint of a segment.
 * A cell is considered covered when its centre is within half_width of the
 * segment line. Already-covered cells are not re-counted.
 */
static void bounce_coverage_grid_mark_segment(bounce_coverage_grid_t *grid,
                                              const bounce_segment_t *segment,
                                              float half_width)
{
    if (grid->cells == NULL || grid->valid_count == 0)
        return;

    point_t a = segment->start;
    point_t b = segment->end;
    float r = half_width;

    // Axis-aligned bounding box of the capsule.
    float sx = (a.x < b.x ? a.x : b.x) - r;
    float ex = (a.x > b.x ? a.x : b.x) + r;
    float sy = (a.y < b.y ? a.y : b.y) - r;
    float ey = (a.y > b.y ? a.y : b.y) + r;

    int col_lo = (int)((sx - grid->origin_x) / grid->cell_size);
    int col_hi = (int)((ex - grid->origin_x) / grid->cell_size);
    int row_lo = (int)((sy - grid->origin_y) / grid->cell_size);
    int row_hi = (int)((ey - grid->origin_y) / grid->cell_size);

    if (col_lo < 0)
        col_lo = 0;
    if (row_lo < 0)
        row_lo = 0;
    if (col_hi >= (int)grid->cols)
        col_hi = (int)grid->cols - 1;
    if (row_hi >= (int)grid->rows)
        row_hi = (int)grid->rows - 1;

    for (int row = row_lo; row <= row_hi; ++row)
    {
        for (int col = col_lo; col <= col_hi; ++col)
        {
            uint32_t idx = (uint32_t)row * grid->cols + (uint32_t)col;
            uint8_t cell = grid->cells[idx];

            if (!(cell & 0x01))
                continue; // not a valid AOI cell
            if (cell & 0x02)
                continue; // already covered

            point_t center = {
                grid->origin_x + ((float)col + 0.5f) * grid->cell_size,
                grid->origin_y + ((float)row + 0.5f) * grid->cell_size};

            if (bounce_metrics_point_to_segment_dist(center, a, b) <= r)
            {
                grid->cells[idx] |= 0x02;
                ++grid->covered_count;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void bounce_update_metrics(bounce_pipeline_context_t *ctx,
                           const bounce_segment_t *segment)
{
    if (ctx == NULL || segment == NULL || ctx->original_env == NULL)
        return;

    // Accumulate distance.
    float dx = segment->end.x - segment->start.x;
    float dy = segment->end.y - segment->start.y;
    ctx->metrics.total_distance += sqrtf(dx * dx + dy * dy);

    // Build the coverage grid once from the original AOI.
    if (!ctx->coverage_grid_ready)
    {
        bounce_coverage_grid_build(&ctx->coverage_grid, ctx->original_env);
        ctx->coverage_grid_ready = true;
    }

    // Mark cells swept by this segment's capsule footprint.
    float half_width = ctx->original_env->path_width * 0.5f;
    bounce_coverage_grid_mark_segment(&ctx->coverage_grid, segment, half_width);

    // Compute coverage ratio — de-overlapped cell count / total valid cells.
    if (ctx->coverage_grid.valid_count > 0)
    {
        float ratio = (float)ctx->coverage_grid.covered_count /
                      (float)ctx->coverage_grid.valid_count;
        ctx->metrics.estimated_coverage = bounce_metrics_clampf(ratio * 100.0f, 0.0f, 100.0f);
    }
    else
    {
        ctx->metrics.estimated_coverage = 0.0f;
    }
}
