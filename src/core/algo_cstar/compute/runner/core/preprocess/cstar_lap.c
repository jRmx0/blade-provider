#include <math.h>
#include "cstar_lap.h"

#define CSTAR_EPSILON 1e-6f

static cstar_lap_t cstar_lap_make(int id, float x)
{
    cstar_lap_t lap = {0};
    lap.id = id;
    lap.x = x;
    lap.node_ids = NULL;
    lap.node_count = 0;
    lap.node_capacity = 0;
    return lap;
}

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

void cstar_lap_generate_full_width(cstar_sampling_front_t *front,
                                   point_t anchor_pos,
                                   float w,
                                   const cstar_environment_t *env)
{
    if (front == NULL || env == NULL || env->operationalBoundary.vertices == NULL || env->operationalBoundary.vertex_count < 3u)
    {
        return;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    float step = (w > CSTAR_EPSILON) ? w : 1.0f;
    float anchor_x = anchor_pos.x;
    if (anchor_x < min_x)
        anchor_x = min_x;
    if (anchor_x > max_x)
        anchor_x = max_x;

    int first_index = (int)ceilf(((min_x - anchor_x) / step) - CSTAR_EPSILON);
    int last_index = (int)floorf(((max_x - anchor_x) / step) + CSTAR_EPSILON);

    int lap_id = 0;
    for (int lap_index = first_index; lap_index <= last_index; ++lap_index)
    {
        float x = anchor_x + ((float)lap_index * step);
        if (x < min_x)
        {
            if ((min_x - x) > CSTAR_EPSILON)
            {
                continue;
            }
            x = min_x;
        }
        if (x > max_x)
        {
            if ((x - max_x) > CSTAR_EPSILON)
            {
                continue;
            }
            x = max_x;
        }

        cvector_push_back(front->laps, cstar_lap_make(lap_id++, x));
    }

    if (front->laps == NULL || cvector_size(front->laps) == 0)
    {
        cvector_push_back(front->laps, cstar_lap_make(0, anchor_x));
    }
}
