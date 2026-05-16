#ifndef CSTAR_SAMPLING_H
#define CSTAR_SAMPLING_H

#include "../../../internal.h"
#include "../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Sampling front
// -------------------------------------------------------------------------

/**
 * The obstacle-free, unsampled portion of the area discovered in iteration i.
 * Holds the set of laps prepared for frontier sample generation.
 */
typedef struct
{
    cvector_vector_type(cstar_lap_t) laps;
} cstar_sampling_front_t;

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * Builds the sampling front for the area newly discovered while the robot
 * travelled from prev_pos to curr_pos.
 *
 * w       - sampling resolution / lap spacing (metres)
 * lap_dir - unit vector parallel to the laps (back-and-forth axis direction)
 *
 * Lap geometry is delegated to the lap-generation module; this function owns
 * the sampling-front container that frontier samples are later placed onto.
 */
cstar_sampling_front_t cstar_create_sampling_front(point_t prev_pos,
                                                   point_t curr_pos,
                                                   float rd,
                                                   float w,
                                                   point_t lap_dir,
                                                   const input_environment_t *env);

/**
 * Places frontier samples on every lap in the sampling front and registers
 * them as new nodes in the RCG. Returns the number of nodes added.
 *
 * delta - minimum spacing multiplier; sample spacing = delta * w (delta >= 1)
 */
int cstar_generate_frontier_samples(cstar_sampling_front_t *front,
                                    cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const input_environment_t *env);

/**
 * Returns true if sample s qualifies as a frontier sample, i.e. the ball
 * B(s, w) contains unknown area and/or an obstacle (Definition III.3).
 */
bool cstar_is_frontier_sample(point_t s, float w, const input_environment_t *env);

/**
 * Frees all memory owned by the sampling front (including its lap vectors).
 */
void cstar_sampling_front_free(cstar_sampling_front_t *front);

#endif // CSTAR_SAMPLING_H
