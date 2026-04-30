#ifndef BCD_POLYGON_OFFSET_H
#define BCD_POLYGON_OFFSET_H

#include <stdint.h>
#include "../../../../../../dependencies/cvector/cvector.h"
#include "../../../internal.h"

/**
 * Computes an inward/outward vertex offset for a polygon using a bisector
 * (miter) method.
 *
 * For CW polygons (zones), a positive offset shrinks the polygon inward.
 * For CCW polygons (obstacles), a positive offset expands the polygon outward.
 *
 * At sharp convex corners the miter length is clamped to 4 * offset to avoid
 * extreme spikes. At reflex (concave) corners the intersection of the two
 * adjacent offset edge lines is emitted as a single point, filling the
 * concavity rather than notching it. If the intersection is too far away or
 * the adjacent edges are parallel, the midpoint of the two edge-normal offsets
 * is used as a fallback. The output vertex count always equals the input count.
 *
 * Returns a newly-allocated cvector of offset points. The caller is
 * responsible for freeing it with cvector_free(). Returns NULL on allocation
 * failure or degenerate input.
 */
cvector_vector_type(point_t) compute_polygon_vertex_offset(
    const point_t *vertices,
    uint32_t count,
    polygon_winding_t winding,
    float offset);

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

#endif // BCD_POLYGON_OFFSET_H
