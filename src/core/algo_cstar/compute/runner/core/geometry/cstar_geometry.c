#include "cstar_geometry.h"

void cstar_lap_boundary_bbox(const cstar_environment_t *env,
                             float *min_x,
                             float *max_x,
                             float *min_y,
                             float *max_y)
{
    *min_x = env->operationalBoundary.vertices[0].x;
    *max_x = env->operationalBoundary.vertices[0].x;
    *min_y = env->operationalBoundary.vertices[0].y;
    *max_y = env->operationalBoundary.vertices[0].y;

    for (uint32_t i = 1; i < env->operationalBoundary.vertex_count; ++i)
    {
        point_t v = env->operationalBoundary.vertices[i];
        if (v.x < *min_x)
            *min_x = v.x;
        if (v.x > *max_x)
            *max_x = v.x;
        if (v.y < *min_y)
            *min_y = v.y;
        if (v.y > *max_y)
            *max_y = v.y;
    }
}
