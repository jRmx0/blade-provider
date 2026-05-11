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
 * inside the zone polygon (slot 0) and outside all obstacle polygons (slots 1+).
 *
 * Checks applied in order:
 *
 *   1a. Multi-sample zone containment: four evenly-spaced interior samples
 *       (t = 0.2, 0.4, 0.6, 0.8) are all tested against the zone polygon.
 *       Using multiple samples catches segments that exit and re-enter the
 *       zone (where a single midpoint would pass).  The zone polygon is used
 *       directly without centroid-nudging — a nudged centroid can land outside
 *       non-convex fields, making the nudge geometrically meaningless.
 *
 *   1b. Multi-sample obstacle exclusion: the same four samples must all lie
 *       outside every obstacle polygon.  This catches chords that tunnel
 *       through a convex obstacle without crossing its edges.
 *
 *   2.  Proper edge crossings: the segment must not properly cross any edge of
 *       any polygon (zone exit or obstacle entry/exit).  Strictly-proper
 *       crossing (open interior intersection) is tested; endpoint touches are
 *       intentionally ignored so valid visibility edges that share a polygon
 *       vertex are not rejected.
 *
 *   3.  Concave-vertex passage (all polygons): for each polygon vertex V that
 *       is collinear with and strictly between p and q, find the first
 *       non-collinear neighbour on each side of the polygon ring.  If those
 *       neighbours lie on opposite sides of line p→q the polygon boundary
 *       genuinely crosses the segment at V → reject.
 *
 *       For the zone (slot 0) this means the path exits the zone at a concave
 *       notch vertex.  For an obstacle (slot 1+) this means the path enters
 *       the obstacle at a concave vertex.
 *
 *       Fix over the previous version: when the neighbour walk finds only
 *       collinear vertices (straight boundary run) it leaves c_prev or c_next
 *       at 0.  The product 0 * anything = 0 is NOT < 0, so the old code
 *       silently passed.  We now treat a zero product as ambiguous and reject,
 *       which is conservative but correct.
 */
static bool vg_segment_is_free(point_t p, point_t q,
                               const cvector_vector_type(point_t) * offset_polys,
                               uint32_t total_polys)
{
    float pq_x = q.x - p.x;
    float pq_y = q.y - p.y;
    float pq_len_sq = pq_x * pq_x + pq_y * pq_y;

    /* Four evenly-spaced interior samples along the segment (avoids endpoints
     * which may sit exactly on a polygon edge). */
    static const float sample_t[4] = {0.2f, 0.4f, 0.6f, 0.8f};

    // Check 1a: every sample must be inside the zone polygon.
    if (total_polys > 0 && offset_polys[0] != NULL)
    {
        const cvector_vector_type(point_t) zverts0 = offset_polys[0];
        if (cvector_size(zverts0) > 0)
        {
            for (int si = 0; si < 4; ++si)
            {
                float t = sample_t[si];
                point_t s = {p.x + t * pq_x, p.y + t * pq_y};
                if (!vg_point_in_polygon(s, zverts0))
                    return false;
            }
        }
    }

    // Check 1b: every sample must be outside every obstacle polygon.
    for (uint32_t pi = 1; pi < total_polys; ++pi)
    {
        if (offset_polys[pi] == NULL)
            continue;
        for (int si = 0; si < 4; ++si)
        {
            float t = sample_t[si];
            point_t s = {p.x + t * pq_x, p.y + t * pq_y};
            if (vg_point_in_polygon(s, offset_polys[pi]))
                return false;
        }
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

    // Check 3: concave-vertex passage for ALL polygons.
    // For each polygon vertex V that is collinear with and strictly between
    // p and q: find the first non-collinear neighbour in each ring direction.
    // If those neighbours are on opposite sides of line p→q the boundary
    // genuinely crosses the segment at V → reject.
    //
    // For slot 0 (zone): opposite sides means the path exits the zone.
    // For slot 1+ (obstacle): opposite sides means the path enters the obstacle.
    //
    // If the neighbour walk exhausts all ring vertices without finding a
    // non-collinear one (degenerate polygon), c_prev or c_next stays 0.
    // A zero product is treated as a crossing (conservative / safe).
    if (pq_len_sq > 1e-12f)
    {
        float eps_cross_sq = 1e-6f * pq_len_sq;

        for (uint32_t pi = 0; pi < total_polys; ++pi)
        {
            const cvector_vector_type(point_t) verts = offset_polys[pi];
            if (verts == NULL)
                continue;
            uint32_t n = (uint32_t)cvector_size(verts);

            for (uint32_t vi = 0; vi < n; ++vi)
            {
                point_t V = verts[vi];

                // Must be strictly between p and q along the segment direction.
                float dp = (V.x - p.x) * pq_x + (V.y - p.y) * pq_y;
                if (dp <= 0.0f || dp >= pq_len_sq)
                    continue;

                // Collinearity test: lateral distance² ≤ ε·|pq|².
                float cross = pq_x * (V.y - p.y) - pq_y * (V.x - p.x);
                if (cross * cross > eps_cross_sq)
                    continue;

                // Walk backwards for first non-collinear predecessor.
                float c_prev = 0.0f;
                for (uint32_t step = 1; step <= n; ++step)
                {
                    point_t Vs = verts[(vi + n - step) % n];
                    float c = pq_x * (Vs.y - p.y) - pq_y * (Vs.x - p.x);
                    if (c * c > eps_cross_sq)
                    {
                        c_prev = c;
                        break;
                    }
                }

                // Walk forwards for first non-collinear successor.
                float c_next = 0.0f;
                for (uint32_t step = 1; step <= n; ++step)
                {
                    point_t Vs = verts[(vi + step) % n];
                    float c = pq_x * (Vs.y - p.y) - pq_y * (Vs.x - p.x);
                    if (c * c > eps_cross_sq)
                    {
                        c_next = c;
                        break;
                    }
                }

                // Opposite sides (c_prev * c_next < 0) → boundary crosses here.
                // Zero product → degenerate / ambiguous → reject conservatively.
                if (c_prev * c_next <= 0.0f)
                    return false;
            }
        }
    }

    return true;
}

// IMPLEMENTATION --- pre-built visibility graph (vg_graph_t) -----------

/*
 * Builds an internal polygon array from `env`.  Caller frees via
 *   for (uint32_t p = 0; p < nav_total; ++p) cvector_free(nav_polys[p]);
 *   va_free(nav_polys);
 * Returns NULL on allocation failure.
 */
static cvector_vector_type(point_t) * vg_build_nav_polys(
                                          const input_environment_t *env, uint32_t *out_total)
{
    uint32_t nav_total = 1 + env->obstacle_count;
    cvector_vector_type(point_t) *nav_polys =
        (cvector_vector_type(point_t) *)va_calloc(
            nav_total, sizeof(cvector_vector_type(point_t)));
    if (!nav_polys)
        return NULL;

    for (uint32_t vi = 0; vi < env->boundary.vertex_count; ++vi)
        cvector_push_back(nav_polys[0], env->boundary.vertices[vi]);

    for (uint32_t k = 0; k < env->obstacle_count; ++k)
        for (uint32_t vi = 0; vi < env->obstacles[k].vertex_count; ++vi)
            cvector_push_back(nav_polys[k + 1], env->obstacles[k].vertices[vi]);

    *out_total = nav_total;
    return nav_polys;
}

/*
 * Epsilon-match: returns the index of the first node within 1e-6 of p, or -1.
 * Since all query points were injected at build time, this is usually exact.
 */
static int vg_find_node(const vg_graph_t *graph, point_t p)
{
    const float EPS2 = 1e-12f; /* (1 µm)² at metre-scale coordinates */
    for (int i = 0; i < graph->n; ++i)
    {
        float dx = graph->nodes[i].x - p.x;
        float dy = graph->nodes[i].y - p.y;
        if (dx * dx + dy * dy < EPS2)
            return i;
    }
    return -1;
}

vg_graph_t *vg_graph_build(const input_environment_t *env,
                           const point_t *extra_nodes, int extra_count)
{
    /* 1. Build polygon array from env. */
    uint32_t nav_total = 0;
    cvector_vector_type(point_t) *nav_polys = vg_build_nav_polys(env, &nav_total);
    if (!nav_polys)
        return NULL;

    /* 2. Collect all nodes: extra_nodes first, then polygon vertices. */
    cvector_vector_type(point_t) nodes = NULL;
    for (int ei = 0; ei < extra_count; ++ei)
        cvector_push_back(nodes, extra_nodes[ei]);
    for (uint32_t pi = 0; pi < nav_total; ++pi)
    {
        if (nav_polys[pi] == NULL)
            continue;
        uint32_t nv = (uint32_t)cvector_size(nav_polys[pi]);
        for (uint32_t vi = 0; vi < nv; ++vi)
            cvector_push_back(nodes, nav_polys[pi][vi]);
    }

    int n = (int)cvector_size(nodes);
    if (n == 0)
    {
        for (uint32_t p = 0; p < nav_total; ++p)
            cvector_free(nav_polys[p]);
        va_free(nav_polys);
        cvector_free(nodes);
        return NULL;
    }

    /* 3. Allocate adjacency matrix and graph struct. */
    bool *adj = (bool *)va_calloc((size_t)n * (size_t)n, sizeof(bool));
    vg_graph_t *graph = (vg_graph_t *)va_malloc(sizeof(vg_graph_t));
    if (!adj || !graph)
    {
        va_free(adj);
        va_free(graph);
        cvector_free(nodes);
        for (uint32_t p = 0; p < nav_total; ++p)
            cvector_free(nav_polys[p]);
        va_free(nav_polys);
        return NULL;
    }

    /* 4. Evaluate all O(n²/2) edges once. */
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
        {
            bool ok = vg_segment_is_free(nodes[i], nodes[j], nav_polys, nav_total);
            adj[i * n + j] = ok;
            adj[j * n + i] = ok;
        }

    /* 5. Release polygon data — the graph only needs nodes + adj. */
    for (uint32_t p = 0; p < nav_total; ++p)
        cvector_free(nav_polys[p]);
    va_free(nav_polys);

    graph->nodes = nodes;
    graph->adj = adj;
    graph->n = n;
    return graph;
}

cvector_vector_type(point_t) vg_graph_query(const vg_graph_t *graph,
                                            point_t from, point_t to)
{
    if (!graph || graph->n == 0)
        return NULL;

    int from_idx = vg_find_node(graph, from);
    int to_idx = vg_find_node(graph, to);
    if (from_idx < 0 || to_idx < 0)
        return NULL;

    if (from_idx == to_idx)
    {
        /* Degenerate: from == to — return a single-point path. */
        cvector_vector_type(point_t) r = NULL;
        cvector_push_back(r, from);
        return r;
    }

    int n = graph->n;
    float *g_cost = (float *)va_malloc((size_t)n * sizeof(float));
    float *f_arr = (float *)va_malloc((size_t)n * sizeof(float));
    int *par = (int *)va_malloc((size_t)n * sizeof(int));
    bool *closed = (bool *)va_calloc((size_t)n, sizeof(bool));
    bool *in_open = (bool *)va_calloc((size_t)n, sizeof(bool));

    if (!g_cost || !f_arr || !par || !closed || !in_open)
    {
        va_free(g_cost);
        va_free(f_arr);
        va_free(par);
        va_free(closed);
        va_free(in_open);
        return NULL;
    }

    const float INF = 1e30f;
    for (int i = 0; i < n; ++i)
    {
        g_cost[i] = INF;
        f_arr[i] = INF;
        par[i] = -1;
    }

    {
        float dx = graph->nodes[from_idx].x - graph->nodes[to_idx].x;
        float dy = graph->nodes[from_idx].y - graph->nodes[to_idx].y;
        g_cost[from_idx] = 0.0f;
        f_arr[from_idx] = sqrtf(dx * dx + dy * dy);
        in_open[from_idx] = true;
    }

    cvector_vector_type(point_t) result = NULL;

    while (true)
    {
        /* Pick the open node with the lowest f (linear scan; n is small). */
        int cur = -1;
        float best = INF;
        for (int i = 0; i < n; ++i)
            if (in_open[i] && f_arr[i] < best)
            {
                best = f_arr[i];
                cur = i;
            }

        if (cur < 0)
            break; /* open set exhausted — no path */

        if (cur == to_idx)
        {
            cvector_vector_type(int) rev = NULL;
            for (int c = cur; c >= 0; c = par[c])
                cvector_push_back(rev, c);
            int rlen = (int)cvector_size(rev);
            for (int k = rlen - 1; k >= 0; --k)
                cvector_push_back(result, graph->nodes[rev[k]]);
            cvector_free(rev);
            break;
        }

        in_open[cur] = false;
        closed[cur] = true;

        for (int nb = 0; nb < n; ++nb)
        {
            if (closed[nb] || nb == cur || !graph->adj[cur * n + nb])
                continue;
            float dx = graph->nodes[nb].x - graph->nodes[cur].x;
            float dy = graph->nodes[nb].y - graph->nodes[cur].y;
            float ng = g_cost[cur] + sqrtf(dx * dx + dy * dy);
            if (ng < g_cost[nb])
            {
                g_cost[nb] = ng;
                float hdx = graph->nodes[nb].x - graph->nodes[to_idx].x;
                float hdy = graph->nodes[nb].y - graph->nodes[to_idx].y;
                f_arr[nb] = ng + sqrtf(hdx * hdx + hdy * hdy);
                par[nb] = cur;
                in_open[nb] = true;
            }
        }
    }

    va_free(g_cost);
    va_free(f_arr);
    va_free(par);
    va_free(closed);
    va_free(in_open);
    return result;
}

void vg_graph_free(vg_graph_t *graph)
{
    if (!graph)
        return;
    cvector_free(graph->nodes);
    va_free(graph->adj);
    va_free(graph);
}

// IMPLEMENTATION --- find_free_space_path / find_free_space_path_ex ------

/*
 * Shared helper: injects from+to alongside extra_nodes, builds a one-shot
 * vg_graph_t, runs A*, then frees the graph.
 */
static cvector_vector_type(point_t) find_free_space_path_impl(
    point_t from, point_t to,
    const input_environment_t *env,
    const point_t *extra_nodes, int extra_count)
{
    /* Prepend from + to so they are registered as graph nodes. */
    int total = extra_count + 2;
    point_t *all = (point_t *)va_malloc((size_t)total * sizeof(point_t));
    if (!all)
        return NULL;
    all[0] = from;
    all[1] = to;
    for (int i = 0; i < extra_count; ++i)
        all[2 + i] = extra_nodes[i];

    vg_graph_t *graph = vg_graph_build(env, all, total);
    va_free(all);
    if (!graph)
        return NULL;

    cvector_vector_type(point_t) result = vg_graph_query(graph, from, to);
    vg_graph_free(graph);
    return result;
}

cvector_vector_type(point_t) find_free_space_path(
    point_t from, point_t to,
    const input_environment_t *env)
{
    return find_free_space_path_impl(from, to, env, NULL, 0);
}

cvector_vector_type(point_t) find_free_space_path_ex(
    point_t from, point_t to,
    const input_environment_t *env,
    const point_t *extra_nodes, int extra_count)
{
    return find_free_space_path_impl(from, to, env, extra_nodes, extra_count);
}
