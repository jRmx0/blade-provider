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

// -------------------------------------------------------------------------
// Environment Preprocessing (One-Time Initialization)
// -------------------------------------------------------------------------

bool cstar_preprocess_environment_laps(cstar_environment_t *env, float path_width)
{
    if (env == NULL || env->operationalBoundary.vertices == NULL || env->operationalBoundary.vertex_count < 3u)
    {
        return false;
    }

    // Idempotency check: laps already generated
    if (env->laps != NULL)
    {
        return false;
    }

    // Compute boundary bounding box
    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    // Initialize laps vector
    cstar_lap_t *laps = NULL;

    // Generate lap positions
    float step = (path_width > CSTAR_EPSILON) ? path_width : 1.0f;
    float anchor_x = env->start_point.x;
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

        cvector_push_back(laps, cstar_lap_make(lap_id++, x));
    }

    // Ensure at least one lap exists
    if (laps == NULL || cvector_size(laps) == 0)
    {
        cvector_push_back(laps, cstar_lap_make(0, anchor_x));
    }

    // Store laps in environment
    env->laps = (void *)laps;
    return true;
}

void cstar_environment_laps_cleanup(cstar_environment_t *env)
{
    if (env == NULL || env->laps == NULL)
    {
        return;
    }

    // Cast void* back to cvector of laps
    cstar_lap_t *laps_vector = (cstar_lap_t *)env->laps;
    int lap_count = cvector_size(laps_vector);

    // Free each lap's node_ids
    for (int i = 0; i < lap_count; ++i)
    {
        if (laps_vector[i].node_ids != NULL)
        {
            cvector_free(laps_vector[i].node_ids);
            laps_vector[i].node_ids = NULL;
        }
    }

    // Free the laps vector itself
    cvector_free(laps_vector);
    env->laps = NULL;
}
