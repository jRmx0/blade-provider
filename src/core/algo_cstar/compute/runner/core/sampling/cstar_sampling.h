#ifndef CSTAR_SAMPLING_H
#define CSTAR_SAMPLING_H

#include "../../../../cstar.h"

int cstar_generate_frontier_samples(cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const cstar_environment_t *env);

#endif // CSTAR_SAMPLING_H
