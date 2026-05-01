#ifndef POLYGON_OFFSET_H
#define POLYGON_OFFSET_H

#include <stdint.h>
#include <stdbool.h>
#include "../core_types.h"
#include "../../../dependencies/cvector/cvector.h"

/**
 * Computes an inward/outward vertex offset for a polygon using a bisector
 * (miter) method.
 *
 * For CW polygons (zones), a positive offset shrinks the polygon inward.
 * For CCW polygons (obstacles), a positive offset expands the polygon outward.
 *
 * At sharp convex corners the behaviour depends on allow_bevel and winding:
 *   - false: miter length is clamped to 4 * offset.
 *   - true + CCW polygon (obstacle): a bevel join is emitted instead — two
 *     separate offset-edge endpoints replace the single miter point.
 *   - true + CW polygon (zone): convex sharp corners stay single-vertex via
 *     clamped miter (no bevel).
 *
 * At reflex (concave) corners, the default is to emit the intersection of the
 * two adjacent offset edge lines as a single point. If that intersection is
 * too far away or edges are parallel, a directionally robust fallback point is
 * used with distance chosen to preserve at least `offset` clearance from both
 * adjacent edges whenever possible.
 *
 * In bevel-enabled mode, CW polygons (zone boundaries) additionally bevel sharp
 * reflex corners (parallel or overlong-miter cases) to two points, matching the
 * sharp-corner headland treatment used at obstacle outer corners.
 *
 * Returns a newly-allocated cvector of offset points. The caller is
 * responsible for freeing it with cvector_free(). Returns NULL on allocation
 * failure or degenerate input.
 */
cvector_vector_type(point_t) compute_polygon_vertex_offset(
    const point_t *vertices,
    uint32_t count,
    polygon_winding_t winding,
    float offset,
    bool allow_bevel);

/**
 * Converts a cvector of offset vertices into a polygon_t (vertices and edges
 * are allocated and populated). The caller owns the resulting polygon and must
 * free it with free_polygon().
 *
 * Returns 0 on success, -1 on NULL/empty input, -2 on allocation failure.
 */
int build_offset_polygon(
    const cvector_vector_type(point_t) offset_vertices,
    polygon_winding_t winding,
    polygon_t *out_polygon);

#endif // POLYGON_OFFSET_H
