#ifndef CSTAR_SAMPLING_H
#define CSTAR_SAMPLING_H

#include "../../../../cstar.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

// -------------------------------------------------------------------------
// Sampling front
// -------------------------------------------------------------------------

/**
 * The obstacle-free, unsampled portion of the area discovered in iteration i.
 * References the pre-generated laps from the environment (stored in env->laps).
 * The laps are generated once during preprocessing and owned by the environment.
 */
typedef struct
{
    const cstar_environment_t *env; // Reference to environment containing pre-generated laps
} cstar_sampling_front_t;

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

/**
 * Creates a sampling front that wraps the environment's pre-generated laps.
 *
 * Assumes that cstar_preprocess_environment_laps() has already been called
 * to populate env->laps. This function is a simple wrapper that references
 * the pre-computed laps from the environment.
 *
 * Returns a sampling front with a reference to the environment.
 */
cstar_sampling_front_t cstar_create_sampling_front(point_t prev_pos,
                                                   point_t curr_pos,
                                                   float rd,
                                                   float w,
                                                   point_t lap_dir,
                                                   const cstar_environment_t *env);

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
                                    const cstar_environment_t *env);

/**
 * Returns true if sample s qualifies as a frontier sample, i.e. the ball
 * B(s, w) contains unknown area and/or an obstacle (Definition III.3).
 */
bool cstar_is_frontier_sample(point_t s, float w, const cstar_environment_t *env);

/**
 * Frees all memory owned by the sampling front (including its lap vectors).
 */
void cstar_sampling_front_free(cstar_sampling_front_t *front);

#endif // CSTAR_SAMPLING_H
