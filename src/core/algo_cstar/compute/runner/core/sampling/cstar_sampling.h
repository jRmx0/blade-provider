#ifndef CSTAR_SAMPLING_H
#define CSTAR_SAMPLING_H

#include "../../../../cstar.h"

typedef struct
{
    const cstar_environment_t *env;
} cstar_sampling_front_t;

/**
 * Builds a sampling front descriptor.
 *
 * For the current implementation, laps are pre-generated in env->laps and the
 * front simply references env.
 */
cstar_sampling_front_t cstar_create_sampling_front(point_t prev_pos,
                                                   point_t curr_pos,
                                                   float rd,
                                                   float w,
                                                   point_t lap_dir,
                                                   const cstar_environment_t *env);

/**
 * Returns true if sample s is a frontier sample for the currently-known
 * operational boundary.
 */
bool cstar_is_frontier_sample(point_t s, float w, const cstar_environment_t *env);

/**
 * Generates frontier samples on pre-generated laps and appends them to the RCG.
 *
 * Returns number of created nodes.
 */
int cstar_generate_frontier_samples(cstar_sampling_front_t *front,
                                    cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const cstar_environment_t *env);

#endif // CSTAR_SAMPLING_H
