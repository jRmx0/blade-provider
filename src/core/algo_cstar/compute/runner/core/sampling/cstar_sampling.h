#ifndef CSTAR_SAMPLING_H
#define CSTAR_SAMPLING_H

#include "../../../../cstar.h"

/**
 * Returns true if sample s is a frontier sample for the currently-known
 * operational boundary.
 */
bool cstar_is_frontier_sample(point_t s, float w, const cstar_environment_t *env);

int cstar_generate_frontier_samples(cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const cstar_environment_t *env);

#endif // CSTAR_SAMPLING_H
