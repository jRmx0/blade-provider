#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "bcd_headland.h"
#include "bcd_polygon_offset.h"
#include "../../../../../../dependencies/cvector/cvector.h"
#include "../../../../../../dependencies/allocator/allocator.h"

// IMPLEMENTATION --- geometry helpers ----------------------------------

/*
 * Squared distance from point P to segment AB (clamped projection).
 */
static float hl_seg_dist_sq(point_t P, point_t A, point_t B)
{
    float abx = B.x - A.x, aby = B.y - A.y;
    float apx = P.x - A.x, apy = P.y - A.y;
    float len_sq = abx * abx + aby * aby;
    if (len_sq < 1e-12f)
        return apx * apx + apy * apy;
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
 * Minimum distance from point P to any edge of the given polygon.
 */
static float point_to_polygon_min_dist(point_t p, const polygon_t *polygon)
{
    float min_dist_sq = 1e30f;
    for (uint32_t i = 0; i < polygon->edge_count; ++i)
    {
        float d = hl_seg_dist_sq(p, polygon->edges[i].begin, polygon->edges[i].end);
        if (d < min_dist_sq)
            min_dist_sq = d;
    }
    return sqrtf(min_dist_sq);
}

/*
 * Returns the index of the vertex in offset_verts closest to ref.
 */
static uint32_t find_nearest_vertex_index(point_t ref,
                                          const cvector_vector_type(point_t) offset_verts)
{
    uint32_t best = 0;
    float best_sq = 1e30f;
    uint32_t n = (uint32_t)cvector_size(offset_verts);
    for (uint32_t i = 0; i < n; ++i)
    {
        float dx = offset_verts[i].x - ref.x;
        float dy = offset_verts[i].y - ref.y;
        float d = dx * dx + dy * dy;
        if (d < best_sq)
        {
            best_sq = d;
            best = i;
        }
    }
    return best;
}

/*
 * Appends the interior vertices of the shorter arc from start_v to end_v
 * (exclusive of both endpoints) on the given offset polygon.  Choosing the
 * shorter of the two possible arcs (forward vs. backward) minimises travel
 * distance and keeps the transit close to the boundary.
 *
 * The caller is responsible for appending the actual start/end points
 * (from_pt / to_pt) around this call so that endpoint duplicates are avoided.
 */
static void append_offset_arc(cvector_vector_type(point_t) * out,
                              const cvector_vector_type(point_t) verts,
                              uint32_t start_v, uint32_t end_v)
{
    uint32_t n = (uint32_t)cvector_size(verts);
    if (n == 0 || start_v == end_v)
        return;

    uint32_t fwd = (end_v - start_v + n) % n; // steps in the forward direction
    uint32_t bwd = n - fwd;                   // steps in the backward direction

    if (fwd <= bwd)
    {
        // Forward arc: vertices start_v+1 … end_v-1
        for (uint32_t step = 1; step < fwd; ++step)
            cvector_push_back(*out, verts[(start_v + step) % n]);
    }
    else
    {
        // Backward arc: vertices start_v-1 … end_v+1
        for (uint32_t step = 1; step < bwd; ++step)
            cvector_push_back(*out, verts[(start_v - step + n) % n]);
    }
}

// IMPLEMENTATION --- headland visibility-graph A* --------------------

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
 * Returns true when segment p→q lies entirely within the headland free-space:
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

/*
 * Finds the shortest collision-free path from `from` to `to` in the headland
 * free-space using a visibility-graph A*.
 *
 * Node set: {from} ∪ {all offset polygon vertices} ∪ {to}.
 * An edge (u, v) is valid when vg_segment_is_free(u, v) is true.
 * Cost and heuristic are Euclidean distances.
 *
 * Returns a cvector of waypoints (caller frees) on success, or NULL when no
 * path exists — caller should fall back to a direct 2-point segment.
 */
static cvector_vector_type(point_t) headland_astar(
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

// IMPLEMENTATION --- trace_polygon_headland ----------------------------

/*
 * The polygon list is laid out as:
 *   index -1  → zone boundary (env->boundary)
 *   index 0+  → env->obstacles[index]
 *
 * Because C arrays are 0-based and we need -1 for the zone, the internal
 * arrays are always accessed with (polygon_index + 1) as the array index,
 * where index 0 maps to the zone and index 1..n map to obstacles.
 *
 * total_count = 1 (zone) + obstacle_count
 */

/*
 * Traces the offset path for one polygon (identified by polygon_index, -1
 * = zone). Starts at offset_verts[start_vertex]. Visits vertices in order
 * until the path comes within `stop_dist` of `stop_polygon`, at which
 * point it returns.
 *
 * While tracing, checks every other polygon for collision (distance <
 * path_width/2 from the original polygon boundary). When a collision is
 * detected, recursively traces the colliding polygon's offset path and
 * appends it to out_path before resuming the current polygon.
 *
 * call_stack / stack_depth prevent re-entering polygons that are already
 * being traced further up the call chain.
 *
 * headland_generated[poly_offset_idx] is set to true once a polygon has
 * been fully handled (either by tracing or by being absorbed into a
 * caller's path via collision transfer).
 *
 * Returns 0 on success, -1 on allocation failure.
 */
static int trace_polygon_headland(
    int polygon_index,
    const input_environment_t *env,
    const cvector_vector_type(point_t) * offset_polys, // array [1 + obstacle_count], zone at [0]
    bool *headland_generated,                          // array [1 + obstacle_count]
    uint32_t start_vertex,
    int stop_polygon_index, // polygon to stop at (-999 = full loop)
    float stop_dist,
    cvector_vector_type(point_t) * out_path,
    int *call_stack,
    int stack_depth);

/*
 * Returns the original polygon_t* for a given polygon_index (-1 = zone).
 */
static const polygon_t *get_original_polygon(int polygon_index,
                                             const input_environment_t *env)
{
    if (polygon_index < 0)
        return &env->boundary;
    return &env->obstacles[(uint32_t)polygon_index];
}

/*
 * Returns the offset vertices cvector for a given polygon_index.
 * Internal layout: zone at slot 0, obstacle i at slot i+1.
 */
static const cvector_vector_type(point_t) get_offset_verts(
    int polygon_index,
    const cvector_vector_type(point_t) * offset_polys)
{
    int slot = polygon_index + 1; // zone: -1+1=0, obs 0: 1, obs 1: 2, …
    return offset_polys[slot];
}

static bool is_in_call_stack(int polygon_index, const int *call_stack, int depth)
{
    for (int i = 0; i < depth; ++i)
        if (call_stack[i] == polygon_index)
            return true;
    return false;
}

static int trace_polygon_headland(
    int polygon_index,
    const input_environment_t *env,
    const cvector_vector_type(point_t) * offset_polys,
    bool *headland_generated,
    uint32_t start_vertex,
    int stop_polygon_index,
    float stop_dist,
    cvector_vector_type(point_t) * out_path,
    int *call_stack,
    int stack_depth)
{
    const cvector_vector_type(point_t) my_verts = get_offset_verts(polygon_index, offset_polys);
    if (my_verts == NULL)
        return 0;

    uint32_t n = (uint32_t)cvector_size(my_verts);
    if (n == 0)
        return 0;

    uint32_t total_polys = 1 + env->obstacle_count;
    float half_width = env->path_width / 2.0f;

    // Add self to call stack
    call_stack[stack_depth] = polygon_index;
    int new_depth = stack_depth + 1;

    uint32_t steps = 0; // safety limit: never visit more than 2 full loops

    uint32_t i = start_vertex;
    while (steps < n * 2)
    {
        point_t curr = my_verts[i];
        cvector_push_back(*out_path, curr);
        ++steps;

        // Check stop condition
        if (stop_polygon_index != -999)
        {
            const polygon_t *stop_poly = get_original_polygon(stop_polygon_index, env);
            float d = point_to_polygon_min_dist(curr, stop_poly);
            if (d < stop_dist && steps > 1) // steps > 1: avoid immediately stopping
                break;
        }
        else
        {
            // Full loop — stop when we return to start (after at least n/2 steps)
            if (steps >= n)
                break;
        }

        // Check all other polygons for collision transfer
        for (uint32_t pi = 0; pi < total_polys; ++pi)
        {
            int other_index = (int)pi - 1; // slot 0 → -1 (zone), slot k → k-1 (obstacle k-1)
            if (other_index == polygon_index)
                continue;
            if (headland_generated[pi])
                continue;
            if (is_in_call_stack(other_index, call_stack, new_depth))
                continue;

            const polygon_t *other_orig = get_original_polygon(other_index, env);
            float dist = point_to_polygon_min_dist(curr, other_orig);
            if (dist < half_width)
            {
                // Transfer to other_index's offset path
                headland_generated[pi] = true; // mark before recursing to prevent double-visit

                const cvector_vector_type(point_t) other_verts =
                    get_offset_verts(other_index, offset_polys);
                uint32_t entry = find_nearest_vertex_index(curr, other_verts);

                int rc = trace_polygon_headland(
                    other_index, env, offset_polys, headland_generated,
                    entry,
                    polygon_index, // stop when reaching back to us
                    half_width,
                    out_path,
                    call_stack, new_depth);

                if (rc != 0)
                    return rc;

                // After the sub-path, bridge back to the nearest vertex of our path
                // (already at current `i`; just continue from here)
                cvector_push_back(*out_path, curr); // re-add current as bridge back
            }
        }

        i = (i + 1) % n;
    }

    return 0;
}

// IMPLEMENTATION --- compute_bcd_headland ------------------------------

int compute_bcd_headland(const input_environment_t *env,
                         bcd_headland_t *headland)
{
    if (env == NULL || headland == NULL)
        return -1;

    memset(headland, 0, sizeof(bcd_headland_t));

    uint32_t total_polys = 1 + env->obstacle_count;
    float half_width = env->path_width / 2.0f;

    // Allocate offset polygon array (slot 0 = zone, slot k+1 = obstacle k)
    cvector_vector_type(point_t) *offset_polys =
        (cvector_vector_type(point_t) *)va_calloc(total_polys,
                                                  sizeof(cvector_vector_type(point_t)));
    if (offset_polys == NULL)
        return -2;

    // Compute offset polygons
    offset_polys[0] = compute_polygon_vertex_offset(
        env->boundary.vertices, env->boundary.vertex_count,
        POLYGON_WINDING_CW, half_width);

    for (uint32_t k = 0; k < env->obstacle_count; ++k)
    {
        offset_polys[k + 1] = compute_polygon_vertex_offset(
            env->obstacles[k].vertices, env->obstacles[k].vertex_count,
            POLYGON_WINDING_CCW, half_width);
    }

    // headland_generated flags (slot 0 = zone, slot k+1 = obstacle k)
    bool *headland_generated = (bool *)va_calloc(total_polys, sizeof(bool));
    if (headland_generated == NULL)
    {
        for (uint32_t p = 0; p < total_polys; ++p)
            cvector_free(offset_polys[p]);
        va_free(offset_polys);
        return -2;
    }

    // Call stack for recursion tracking (max depth = total_polys)
    int *call_stack = (int *)va_malloc(total_polys * sizeof(int));
    if (call_stack == NULL)
    {
        for (uint32_t p = 0; p < total_polys; ++p)
            cvector_free(offset_polys[p]);
        va_free(offset_polys);
        va_free(headland_generated);
        return -2;
    }

    // --- Trace zone headland (index = -1, slot 0) ---
    if (offset_polys[0] != NULL)
    {
        headland_generated[0] = true;

        headland_section_t zone_section;
        zone_section.source_index = -1;
        zone_section.path = NULL;
        zone_section.nav = NULL;

        int rc = trace_polygon_headland(
            -1, env, offset_polys, headland_generated,
            0,    // start vertex
            -999, // full loop
            0.0f,
            &zone_section.path,
            call_stack, 0);

        if (rc != 0)
        {
            cvector_free(zone_section.path);
            // cleanup below
        }
        else
        {
            // Close the loop: append the first point as the final point.
            if (zone_section.path != NULL && cvector_size(zone_section.path) > 0)
                cvector_push_back(zone_section.path, zone_section.path[0]);

            cvector_push_back(headland->sections, zone_section);
        }
    }

    // --- Trace any obstacle headlands not yet covered ---
    for (uint32_t k = 0; k < env->obstacle_count; ++k)
    {
        uint32_t slot = k + 1;
        if (headland_generated[slot])
            continue;
        if (offset_polys[slot] == NULL)
            continue;

        headland_generated[slot] = true;

        headland_section_t obs_section;
        obs_section.source_index = (int)k;
        obs_section.path = NULL;
        obs_section.nav = NULL;

        int rc = trace_polygon_headland(
            (int)k, env, offset_polys, headland_generated,
            0,
            -999,
            0.0f,
            &obs_section.path,
            call_stack, 0);

        if (rc == 0)
        {
            // Close the loop: append the first point as the final point.
            if (obs_section.path != NULL && cvector_size(obs_section.path) > 0)
                cvector_push_back(obs_section.path, obs_section.path[0]);

            cvector_push_back(headland->sections, obs_section);
        }
        else
        {
            cvector_free(obs_section.path);
        }
    }

    // --- Inter-section transit paths ---
    // Same source polygon: arc-trace the shorter arc along the shared offset
    // polygon — always collision-free within the headland strip.
    // Different source polygons: visibility-graph A* in the headland free-space
    // (inside zone offset, outside all obstacle offsets) to guarantee the path
    // avoids all obstacles regardless of the field geometry.
    // The last section's nav (headland → first coverage point) is filled later
    // in bcd_runner.c once the BCD motion plan is available.
    {
        int sec_count = (int)cvector_size(headland->sections);
        for (int i = 0; i < sec_count - 1; ++i)
        {
            const cvector_vector_type(point_t) cur_path = headland->sections[i].path;
            const cvector_vector_type(point_t) next_path = headland->sections[i + 1].path;
            if (cur_path == NULL || cvector_size(cur_path) == 0)
                continue;
            if (next_path == NULL || cvector_size(next_path) == 0)
                continue;

            point_t from_pt = cur_path[cvector_size(cur_path) - 1];
            point_t to_pt = next_path[0];

            int from_src = headland->sections[i].source_index;
            int to_src = headland->sections[i + 1].source_index;

            cvector_vector_type(point_t) nav = NULL;

            if (from_src == to_src)
            {
                // Same offset polygon: arc-trace the shorter boundary arc.
                // Guaranteed collision-free — the arc follows the polygon boundary.
                const cvector_vector_type(point_t) from_verts = get_offset_verts(from_src, offset_polys);
                if (from_verts != NULL)
                {
                    uint32_t from_v = find_nearest_vertex_index(from_pt, from_verts);
                    uint32_t to_v = find_nearest_vertex_index(to_pt, from_verts);
                    cvector_push_back(nav, from_pt);
                    cvector_push_back(nav, from_verts[from_v]);
                    append_offset_arc(&nav, from_verts, from_v, to_v);
                    cvector_push_back(nav, from_verts[to_v]);
                    cvector_push_back(nav, to_pt);
                }
            }
            else
            {
                // Different source polygons: run A* on the visibility graph built
                // from all offset polygon vertices.  This correctly handles fields
                // with multiple isolated obstacles where a direct bridge would clip
                // through a third obstacle.
                nav = headland_astar(from_pt, to_pt, offset_polys, total_polys);
            }

            if (nav == NULL)
            {
                // Fallback (A* found no path or same-poly verts were NULL):
                // direct 2-point segment — always terminate the loop.
                cvector_push_back(nav, from_pt);
                cvector_push_back(nav, to_pt);
            }

            headland->sections[i].nav = nav;
        }
    }

    // --- Build reduced geometry polygons for BCD ---
    //
    // The BCD area boundary must be offset inward (zone) / outward (obstacles)
    // by bcd_shrink from the original polygon edges.
    //
    // headland_coverage_offset is measured from the headland path centreline
    // (which sits at half_width from the original edge), so the total offset
    // from the original edge is:
    //
    //   bcd_shrink = half_width + headland_coverage_offset
    //
    // offset_polys[] remain at half_width and are used only for headland path
    // tracing and A* transit — they must not be reused here.
    float bcd_shrink = half_width + env->headland_coverage_offset;

    cvector_vector_type(point_t) bcd_zone_verts = compute_polygon_vertex_offset(
        env->boundary.vertices, env->boundary.vertex_count,
        POLYGON_WINDING_CW, bcd_shrink);
    if (bcd_zone_verts != NULL)
    {
        int rc = build_offset_polygon(bcd_zone_verts, POLYGON_WINDING_CW,
                                      &headland->shrunken_zone);
        if (rc != 0)
        {
            printf("compute_bcd_headland: failed to build shrunken zone polygon (%d)\n", rc);
        }
        cvector_free(bcd_zone_verts);
    }

    if (env->obstacle_count > 0)
    {
        headland->expanded_obstacles =
            (polygon_t *)va_calloc((size_t)env->obstacle_count, sizeof(polygon_t));
        if (headland->expanded_obstacles != NULL)
        {
            headland->expanded_obstacle_count = env->obstacle_count;
            for (uint32_t k = 0; k < env->obstacle_count; ++k)
            {
                cvector_vector_type(point_t) bcd_obs_verts = compute_polygon_vertex_offset(
                    env->obstacles[k].vertices, env->obstacles[k].vertex_count,
                    POLYGON_WINDING_CCW, bcd_shrink);
                if (bcd_obs_verts != NULL)
                {
                    int rc = build_offset_polygon(bcd_obs_verts, POLYGON_WINDING_CCW,
                                                  &headland->expanded_obstacles[k]);
                    if (rc != 0)
                    {
                        printf("compute_bcd_headland: failed to build expanded obstacle %u polygon (%d)\n", k, rc);
                    }
                    cvector_free(bcd_obs_verts);
                }
            }
        }
    }

    // Cleanup temporaries
    for (uint32_t p = 0; p < total_polys; ++p)
        cvector_free(offset_polys[p]);
    va_free(offset_polys);
    va_free(headland_generated);
    va_free(call_stack);

    return 0;
}

// IMPLEMENTATION --- free_bcd_headland ---------------------------------

void free_bcd_headland(bcd_headland_t *headland)
{
    if (headland == NULL)
        return;

    if (headland->sections != NULL)
    {
        int count = (int)cvector_size(headland->sections);
        for (int i = 0; i < count; ++i)
        {
            cvector_free(headland->sections[i].path);
            cvector_free(headland->sections[i].nav);
        }
        cvector_free(headland->sections);
        headland->sections = NULL;
    }

    free_polygon(&headland->shrunken_zone);

    if (headland->expanded_obstacles != NULL)
    {
        for (uint32_t k = 0; k < headland->expanded_obstacle_count; ++k)
            free_polygon(&headland->expanded_obstacles[k]);
        va_free(headland->expanded_obstacles);
        headland->expanded_obstacles = NULL;
    }
    headland->expanded_obstacle_count = 0;
}
