#include <math.h>
#include <stdint.h>
#include "bcd_polygon_offset.h"
#include "../../../../../../dependencies/allocator/allocator.h"
#include "../../../../../../dependencies/cvector/cvector.h"

// IMPLEMENTATION --- helpers -------------------------------------------

/*
 * Returns the unit inward normal of an edge, taking winding into account.
 *
 * For a CW polygon (boundary) the interior is on the right side of the
 * directed edge (begin→end), so the inward normal is the right-hand normal.
 *
 * For a CCW polygon (obstacle) the interior is on the left side, so the
 * inward normal is the left-hand normal.
 *
 * In both cases we want to move vertices *away* from the polygon interior
 * consistently so that CW shrinks and CCW expands. We define "offset
 * direction" as the right-hand normal for CW and the left-hand normal for
 * CCW (both pointing into the interior of the offset strip).
 */
static point_t edge_offset_normal(point_t begin, point_t end, polygon_winding_t winding)
{
    float dx = end.x - begin.x;
    float dy = end.y - begin.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-9f)
    {
        point_t zero = {0.0f, 0.0f};
        return zero;
    }
    dx /= len;
    dy /= len;

    // Left-hand normal in screen coords (Y-down): (-dy, dx).
    // For CW (zone): left-hand is inward → shrinks the polygon.
    // For CCW (obstacle): left-hand is outward → expands the polygon.
    // Both cases use the same formula; winding is kept for documentation.
    (void)winding;
    point_t n = {-dy, dx};
    return n;
}

/*
 * Checks whether the turn at vertex `curr` (from prev→curr→next) is a reflex
 * (concave) vertex relative to the polygon winding.
 *
 * Cross product sign: positive = left turn, negative = right turn.
 * For CW polygons, a right turn (negative cross) is convex; left turn (positive cross) is reflex.
 * For CCW polygons, a left turn (positive cross) is convex; right turn (negative cross) is reflex.
 */
static bool is_reflex_vertex(point_t prev, point_t curr, point_t next,
                             polygon_winding_t winding)
{
    float cross = (curr.x - prev.x) * (next.y - prev.y) - (curr.y - prev.y) * (next.x - prev.x);

    // In screen coords (Y-down), positive cross = CW turn on screen.
    // CW polygon: CW turn (cross > 0) = convex → reflex when cross < 0.
    // CCW polygon: CCW turn (cross < 0) = convex → reflex when cross > 0.
    if (winding == POLYGON_WINDING_CW)
        return cross < 0.0f;
    else
        return cross > 0.0f;
}

// IMPLEMENTATION --- compute_polygon_vertex_offset ---------------------

cvector_vector_type(point_t) compute_polygon_vertex_offset(
    const point_t *vertices,
    uint32_t count,
    polygon_winding_t winding,
    float offset)
{
    if (vertices == NULL || count < 3 || offset <= 0.0f)
        return NULL;

    cvector_vector_type(point_t) result = NULL;
    const float max_miter = 4.0f * offset;

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t prev_i = (i == 0) ? count - 1 : i - 1;
        uint32_t next_i = (i + 1) % count;

        point_t prev = vertices[prev_i];
        point_t curr = vertices[i];
        point_t next = vertices[next_i];

        // Edge normals for the two edges meeting at this vertex
        point_t n1 = edge_offset_normal(prev, curr, winding); // incoming edge
        point_t n2 = edge_offset_normal(curr, next, winding); // outgoing edge

        if (is_reflex_vertex(prev, curr, next, winding))
        {
            // At a reflex vertex emit two separate offset points (one per
            // adjacent edge) to avoid the miter crossing the polygon boundary.
            point_t p1 = {curr.x + n1.x * offset, curr.y + n1.y * offset};
            point_t p2 = {curr.x + n2.x * offset, curr.y + n2.y * offset};
            cvector_push_back(result, p1);
            cvector_push_back(result, p2);
        }
        else
        {
            // Bisector of the two edge normals
            float bx = n1.x + n2.x;
            float by = n1.y + n2.y;
            float blen = sqrtf(bx * bx + by * by);

            point_t offset_pt;
            if (blen < 1e-9f)
            {
                // Anti-parallel normals (180° turn) — straight-through, use n2
                offset_pt.x = curr.x + n2.x * offset;
                offset_pt.y = curr.y + n2.y * offset;
            }
            else
            {
                // Miter scale = offset / dot(bisector_unit, n1)
                // which equals offset / cos(half_angle)
                float bux = bx / blen;
                float buy = by / blen;
                float dot = bux * n1.x + buy * n1.y;
                float scale = (dot > 1e-6f) ? (offset / dot) : max_miter;
                if (scale > max_miter)
                    scale = max_miter;

                offset_pt.x = curr.x + bux * scale;
                offset_pt.y = curr.y + buy * scale;
            }
            cvector_push_back(result, offset_pt);
        }
    }

    return result;
}

// IMPLEMENTATION --- build_offset_polygon ------------------------------

int build_offset_polygon(
    const cvector_vector_type(point_t) offset_vertices,
    polygon_winding_t winding,
    polygon_t *out_polygon)
{
    if (out_polygon == NULL || offset_vertices == NULL)
        return -1;

    uint32_t vc = (uint32_t)cvector_size(offset_vertices);
    if (vc < 3)
        return -1;

    point_t *verts = (point_t *)va_malloc((size_t)vc * sizeof(point_t));
    if (verts == NULL)
        return -2;

    for (uint32_t i = 0; i < vc; ++i)
        verts[i] = offset_vertices[i];

    polygon_edge_t *edges = (polygon_edge_t *)va_malloc((size_t)vc * sizeof(polygon_edge_t));
    if (edges == NULL)
    {
        va_free(verts);
        return -2;
    }

    for (uint32_t i = 0; i < vc; ++i)
    {
        uint32_t next = (i + 1) % vc;
        edges[i].begin = verts[i];
        edges[i].end = verts[next];
    }

    out_polygon->winding = winding;
    out_polygon->vertices = verts;
    out_polygon->vertex_count = vc;
    out_polygon->edges = edges;
    out_polygon->edge_count = vc;

    return 0;
}
