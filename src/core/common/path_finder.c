#include <math.h>
#include <stdbool.h>
#include "path_finder.h"
#include "polygon_offset.h"
#include "../../../dependencies/cvector/cvector.h"
#include "../../../dependencies/allocator/allocator.h"

// IMPLEMENTATION --- visibility-graph helpers --------------------------

/*
 * 2-D cross product of vectors (a→b) and (a→c).
 */
static float vg_cross2d(point_t a, point_t b, point_t c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/*
 * Returns true when segments (a,b) and (c,d) properly cross — their open
 * interiors intersect.  Touching at endpoints is not counted so that
 * visibility-graph edges that share a polygon vertex are not rejected.
 */
static bool vg_segments_properly_intersect(point_t a, point_t b,
                                           point_t c, point_t d)
{
    float d1 = vg_cross2d(c, d, a);
    float d2 = vg_cross2d(c, d, b);
    float d3 = vg_cross2d(a, b, c);
    float d4 = vg_cross2d(a, b, d);
    if (((d1 > 0.0f && d2 < 0.0f) || (d1 < 0.0f && d2 > 0.0f)) &&
        ((d3 > 0.0f && d4 < 0.0f) || (d3 < 0.0f && d4 > 0.0f)))
        return true;
    return false;
}

/*
 * Ray-casting point-in-polygon test.  Returns true when p lies strictly
 * inside the polygon defined by verts.
 */
static bool vg_point_in_polygon(point_t p,
                                const cvector_vector_type(point_t) verts)
{
    uint32_t n = (uint32_t)cvector_size(verts);
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

/*
 * Returns true when segment p→q lies entirely within the free space:
 * inside the zone offset polygon (slot 0) and outside all obstacle offset
 * polygons (slots 1+).
 *
 * Two complementary checks are used:
 *
 *   1. Midpoint-in-obstacle: catches chords that pass through a convex
 *      obstacle polygon without crossing its edges (e.g. a chord between two
 *      non-adjacent obstacle-offset vertices).  Only obstacle slots are tested
 *      because points on the zone boundary make the zone PIP test unreliable.
 *
 *   2. No proper edge crossings: detects any segment that exits the zone or
 *      crosses into/out-of an obstacle.
 */
static bool vg_segment_is_free(point_t p, point_t q,
                               const cvector_vector_type(point_t) * offset_polys,
                               uint32_t total_polys)
{
    point_t mid = {(p.x + q.x) * 0.5f, (p.y + q.y) * 0.5f};

    // Midpoint must be outside every obstacle offset polygon.
    for (uint32_t pi = 1; pi < total_polys; ++pi)
    {
        if (offset_polys[pi] == NULL)
            continue;
        if (vg_point_in_polygon(mid, offset_polys[pi]))
            return false;
    }

    // Segment must not properly cross any polygon boundary edge.
    for (uint32_t pi = 0; pi < total_polys; ++pi)
    {
        const cvector_vector_type(point_t) verts = offset_polys[pi];
        if (verts == NULL)
            continue;
        uint32_t n = (uint32_t)cvector_size(verts);
        for (uint32_t i = 0; i < n; ++i)
        {
            if (vg_segments_properly_intersect(p, q, verts[i], verts[(i + 1) % n]))
                return false;
        }
    }
    return true;
}

// IMPLEMENTATION --- free_space_astar ----------------------------------

/*
 * Finds the shortest collision-free path from `from` to `to` using a
 * visibility-graph A* over the given pre-built offset polygon array.
 *
 * Node set: {from} ∪ {all offset polygon vertices} ∪ {to}.
 * An edge (u, v) is valid when vg_segment_is_free(u, v) is true.
 * Cost and heuristic are Euclidean distances.
 *
 * Returns a cvector of waypoints (caller frees) on success, or NULL when
 * no path exists.
 */
static cvector_vector_type(point_t) free_space_astar(
    point_t from, point_t to,
    const cvector_vector_type(point_t) * offset_polys,
    uint32_t total_polys)
{
    // Build node list: from → all polygon vertices → to.
    cvector_vector_type(point_t) nodes = NULL;
    cvector_push_back(nodes, from);
    for (uint32_t pi = 0; pi < total_polys; ++pi)
    {
        if (offset_polys[pi] == NULL)
            continue;
        uint32_t nv = (uint32_t)cvector_size(offset_polys[pi]);
        for (uint32_t vi = 0; vi < nv; ++vi)
            cvector_push_back(nodes, offset_polys[pi][vi]);
    }
    int goal = (int)cvector_size(nodes); // index of the `to` node
    cvector_push_back(nodes, to);
    int n = goal + 1;

    float *g = (float *)va_malloc((size_t)n * sizeof(float));
    float *f_arr = (float *)va_malloc((size_t)n * sizeof(float));
    int *par = (int *)va_malloc((size_t)n * sizeof(int));
    bool *closed = (bool *)va_calloc((size_t)n, sizeof(bool));
    bool *in_open = (bool *)va_calloc((size_t)n, sizeof(bool));

    if (!g || !f_arr || !par || !closed || !in_open)
    {
        va_free(g);
        va_free(f_arr);
        va_free(par);
        va_free(closed);
        va_free(in_open);
        cvector_free(nodes);
        return NULL;
    }

    const float INF = 1e30f;
    for (int i = 0; i < n; ++i)
    {
        g[i] = INF;
        f_arr[i] = INF;
        par[i] = -1;
    }

    g[0] = 0.0f;
    {
        float dx = nodes[0].x - to.x, dy = nodes[0].y - to.y;
        f_arr[0] = sqrtf(dx * dx + dy * dy);
    }
    in_open[0] = true;

    cvector_vector_type(point_t) result = NULL;

    while (true)
    {
        // Pick the open node with the lowest f (linear scan — n is small: ~20-100).
        int cur = -1;
        float best = INF;
        for (int i = 0; i < n; ++i)
        {
            if (in_open[i] && f_arr[i] < best)
            {
                best = f_arr[i];
                cur = i;
            }
        }
        if (cur < 0)
            break; // exhausted open set, no path

        if (cur == goal)
        {
            // Reconstruct path from goal back to start, then reverse.
            cvector_vector_type(int) rev = NULL;
            for (int c = cur; c >= 0; c = par[c])
                cvector_push_back(rev, c);
            int rlen = (int)cvector_size(rev);
            for (int k = rlen - 1; k >= 0; --k)
                cvector_push_back(result, nodes[rev[k]]);
            cvector_free(rev);
            break;
        }

        in_open[cur] = false;
        closed[cur] = true;

        for (int nb = 0; nb < n; ++nb)
        {
            if (closed[nb] || nb == cur)
                continue;
            if (!vg_segment_is_free(nodes[cur], nodes[nb], offset_polys, total_polys))
                continue;

            float dx = nodes[nb].x - nodes[cur].x;
            float dy = nodes[nb].y - nodes[cur].y;
            float ng = g[cur] + sqrtf(dx * dx + dy * dy);
            if (ng < g[nb])
            {
                g[nb] = ng;
                float hdx = nodes[nb].x - to.x, hdy = nodes[nb].y - to.y;
                f_arr[nb] = ng + sqrtf(hdx * hdx + hdy * hdy);
                par[nb] = cur;
                in_open[nb] = true;
            }
        }
    }

    va_free(g);
    va_free(f_arr);
    va_free(par);
    va_free(closed);
    va_free(in_open);
    cvector_free(nodes);
    return result;
}

// IMPLEMENTATION --- find_free_space_path ------------------------------

cvector_vector_type(point_t) find_free_space_path(
    point_t from, point_t to,
    const input_environment_t *env,
    float offset)
{
    uint32_t nav_total = 1 + env->obstacle_count;
    cvector_vector_type(point_t) *nav_polys =
        (cvector_vector_type(point_t) *)va_calloc(nav_total,
                                                  sizeof(cvector_vector_type(point_t)));
    if (nav_polys == NULL)
        return NULL;

    nav_polys[0] = compute_polygon_vertex_offset(
        env->boundary.vertices, env->boundary.vertex_count,
        POLYGON_WINDING_CW, offset);

    for (uint32_t k = 0; k < env->obstacle_count; ++k)
        nav_polys[k + 1] = compute_polygon_vertex_offset(
            env->obstacles[k].vertices, env->obstacles[k].vertex_count,
            POLYGON_WINDING_CCW, offset);

    cvector_vector_type(point_t) result = free_space_astar(from, to, nav_polys, nav_total);

    for (uint32_t p = 0; p < nav_total; ++p)
        cvector_free(nav_polys[p]);
    va_free(nav_polys);

    return result;
}
