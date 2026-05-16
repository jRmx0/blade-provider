/**
 * debug_serialize.h
 *
 * Shared helper for serialising algorithm output into the standard
 * debug-session segment format expected by the blade-terminal debug API.
 *
 * A "debug segment" is one step unit as exposed by POST /compute/debug/:id/step:
 *
 *   {
 *     "id":   <int>,
 *     "type": "<string>",
 *     "path": [ { "id": <int>, "point": { "x": <float>, "y": <float> } }, ... ]
 *   }
 *
 * Algorithms call debug_build_segment() to produce individual cJSON segment
 * objects, then append them to the coveragePathPlan.segments array.
 * This ensures every algorithm emits the same wire shape.
 */

#ifndef DEBUG_SERIALIZE_H
#define DEBUG_SERIALIZE_H

#include "../../../dependencies/cJSON/cJSON.h"

/**
 * debug_build_segment — build one cJSON segment object.
 *
 * @param id         Zero-based segment index within the result.
 * @param seg_type   Segment classification string (e.g. "coverage", "transition").
 * @param xs         Array of X world-coordinates for the path waypoints.
 * @param ys         Array of Y world-coordinates for the path waypoints.
 * @param count      Number of waypoints (xs and ys must each have at least count elements).
 *
 * @return  A newly allocated cJSON object owned by the caller, or NULL on OOM.
 *          Free with cJSON_Delete() if not transferred to a parent array.
 */
cJSON *debug_build_segment(int id, const char *seg_type,
                           const float *xs, const float *ys, int count);

#endif /* DEBUG_SERIALIZE_H */
