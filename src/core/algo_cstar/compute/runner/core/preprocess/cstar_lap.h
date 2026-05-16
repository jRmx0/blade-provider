#ifndef CSTAR_LAP_H
#define CSTAR_LAP_H

#include "../cstar_sampling.h"

/**
 * Computes the axis-aligned bounding box of the environment boundary.
 */
void cstar_lap_boundary_bbox(const input_environment_t *env,
                             float *min_x,
                             float *max_x,
                             float *min_y,
                             float *max_y);

/**
 * Populates the sampling front with a vertical lap lattice anchored at
 * anchor_pos.x and spanning the full horizontal extent of the environment.
 *
 * Laps are emitted in strictly increasing x order so lap IDs preserve the
 * left-to-right semantics used by the RCG.
 */
void cstar_lap_generate_full_width(cstar_sampling_front_t *front,
                                   point_t anchor_pos,
                                   float w,
                                   const input_environment_t *env);

#endif // CSTAR_LAP_H
