#include <math.h>
#include "cstar_runner_math.h"

static bool cstar_points_equal(point_t a, point_t b)
{
    return fabsf(a.x - b.x) < 1e-5f && fabsf(a.y - b.y) < 1e-5f;
}

static float cstar_runner_dist(point_t a, point_t b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}
