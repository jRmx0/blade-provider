#ifndef CSTAR_RUNNER_MATH_H
#define CSTAR_RUNNER_MATH_H

#include "../../../../core_types.h"

/**
 * Check if two points are equal within floating-point tolerance.
 */
static bool cstar_points_equal(point_t a, point_t b);

/**
 * Calculate Euclidean distance between two points.
 */
static float cstar_runner_dist(point_t a, point_t b);

#endif // CSTAR_RUNNER_MATH_H
