#ifndef CSTAR_GEOMETRY_H
#define CSTAR_GEOMETRY_H

#include "../../../../cstar.h"

/**
 * Computes the axis-aligned bounding box of the environment boundary.
 */
void cstar_lap_boundary_bbox(const cstar_environment_t *env,
                             float *min_x,
                             float *max_x,
                             float *min_y,
                             float *max_y);

#endif // CSTAR_GEOMETRY_H
