#include <math.h>
#include <stdbool.h>
#include "path_finder.h"
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
 * Four complementary checks are used:
 *
 *   1a. Midpoint-in-zone: the test point is nudged 1e-4 of the way toward the
 *       zone centroid so that boundary-midpoints (zone-edge segments) are not
 *       falsely rejected, while segments whose midpoint is genuinely outside
 *       the zone — including the "closer-vertex-on-the-other-side" shortcut
 *       where p or q is outside the zone offset polygon and the segment ends
 *       at a zone corner vertex without properly crossing any edge — are
 *       correctly rejected.
 *
 *   1b. Midpoint-in-obstacle: catches chords that pass through a convex
 *       obstacle polygon without crossing its edges.
 *
 *   2.  No proper edge crossings: detects any segment that exits the zone or
 *       crosses into/out-of an obstacle.
 *
 *   3.  Zone-vertex passage: the proper-crossing check intentionally ignores
 *       vertex-touches so that valid visibility edges that share a zone vertex
 *       are not rejected.  This creates a gap for segments that clip through a
 *       concave zone offset vertex V — lying collinearly between p and q with
 *       V's zone neighbours on opposite sides of line p→q.  When an immediate
 *       neighbour is itself collinear, we walk outward until the first
 *       non-collinear vertex in each direction is found.
 */
static bool vg_segment_is_free(point_t p, point_t q,
                               const cvector_vector_type(point_t) * offset_polys,
                               uint32_t total_polys)
{
    float pq_x = q.x - p.x;
    float pq_y = q.y - p.y;
    float pq_len_sq = pq_x * pq_x + pq_y * pq_y;

    point_t mid = {(p.x + q.x) * 0.5f, (p.y + q.y) * 0.5f};

    // Check 1a: nudged midpoint must be inside the zone offset polygon.
    if (total_polys > 0 && offset_polys[0] != NULL)
    {
        const cvector_vector_type(point_t) zverts0 = offset_polys[0];
        uint32_t zn0 = (uint32_t)cvector_size(zverts0);
        if (zn0 > 0)
        {
            // Compute vertex-average centroid (good enough for convex / mildly
            // non-convex zone offset polygons).
            float cx = 0.0f, cy = 0.0f;
            for (uint32_t vi = 0; vi < zn0; ++vi)
            {
                cx += zverts0[vi].x;
                cy += zverts0[vi].y;
            }
            cx /= (float)zn0;
            cy /= (float)zn0;

            // Nudge 0.01 % of the way toward the centroid so a midpoint that
            // lands exactly on a zone edge is pulled strictly inside, avoiding
            // ray-casting boundary ambiguity.
            const float nudge = 1e-4f;
            point_t test_mid = {
                mid.x + nudge * (cx - mid.x),
                mid.y + nudge * (cy - mid.y)};
            if (!vg_point_in_polygon(test_mid, zverts0))
                return false;
        }
    }

    // Check 1b: midpoint must be outside every obstacle offset polygon.
    for (uint32_t pi = 1; pi < total_polys; ++pi)
    {
        if (offset_polys[pi] == NULL)
            continue;
        if (vg_point_in_polygon(mid, offset_polys[pi]))
            return false;
    }

    // Check 2: segment must not properly cross any polygon boundary edge.
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

    // Check 3: zone-vertex passage (slot 0 only).
    // For each zone offset vertex V strictly between p and q on the segment:
    //   Walk outward in each direction around the zone polygon to find the
    //   first non-collinear neighbour (handles collinear-edge runs where an
    //   immediate neighbour also lies on the segment line, giving c = 0).
    //   If those two non-collinear neighbours are on opposite sides of line
    //   p→q the zone boundary genuinely crosses the segment at V → reject.
    if (total_polys > 0 && offset_polys[0] != NULL && pq_len_sq > 1e-12f)
    {
        const cvector_vector_type(point_t) zverts = offset_polys[0];
        uint32_t zn = (uint32_t)cvector_size(zverts);
        float eps_cross_sq = 1e-6f * pq_len_sq;

        for (uint32_t vi = 0; vi < zn; ++vi)
        {
            point_t V = zverts[vi];

            // Skip vertices coinciding with or beyond a segment endpoint.
            float dp = (V.x - p.x) * pq_x + (V.y - p.y) * pq_y;
            if (dp <= 0.0f || dp >= pq_len_sq)
                continue;

            // Collinearity: |cross(pq, pV)|² ≤ 1e-6 · |pq|²  (≈ 1 mm lateral).
            float cross = pq_x * (V.y - p.y) - pq_y * (V.x - p.x);
            if (cross * cross > eps_cross_sq)
                continue;

            // Walk backwards to find the first non-collinear predecessor.
            float c_prev = 0.0f;
            for (uint32_t step = 1; step <= zn; ++step)
            {
                point_t Vs = zverts[(vi + zn - step) % zn];
                float c = pq_x * (Vs.y - p.y) - pq_y * (Vs.x - p.x);
                if (c * c > eps_cross_sq)
                {
                    c_prev = c;
                    break;
                }
            }

            // Walk forwards to find the first non-collinear successor.
            float c_next = 0.0f;
            for (uint32_t step = 1; step <= zn; ++step)
            {
                point_t Vs = zverts[(vi + step) % zn];
                float c = pq_x * (Vs.y - p.y) - pq_y * (Vs.x - p.x);
                if (c * c > eps_cross_sq)
                {
                    c_next = c;
                    break;
                }
            }

            if (c_prev * c_next < 0.0f)
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
    const input_environment_t *env)
{
    uint32_t nav_total = 1 + env->obstacle_count;
    cvector_vector_type(point_t) *nav_polys =
        (cvector_vector_type(point_t) *)va_calloc(nav_total,
                                                  sizeof(cvector_vector_type(point_t)));
    if (nav_polys == NULL)
        return NULL;

    for (uint32_t vi = 0; vi < env->boundary.vertex_count; ++vi)
        cvector_push_back(nav_polys[0], env->boundary.vertices[vi]);

    for (uint32_t k = 0; k < env->obstacle_count; ++k)
        for (uint32_t vi = 0; vi < env->obstacles[k].vertex_count; ++vi)
            cvector_push_back(nav_polys[k + 1], env->obstacles[k].vertices[vi]);

    cvector_vector_type(point_t) result = free_space_astar(from, to, nav_polys, nav_total);

    for (uint32_t p = 0; p < nav_total; ++p)
        cvector_free(nav_polys[p]);
    va_free(nav_polys);

    return result;
}
