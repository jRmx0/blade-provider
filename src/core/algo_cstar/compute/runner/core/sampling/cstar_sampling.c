#include <math.h>
#include <stdint.h>
#include "cstar_sampling.h"
#include "../geometry/cstar_geometry.h"
#include "../rcg/cstar_rcg.h"
#include "../../../../../../../dependencies/cvector/cvector.h"

#define CSTAR_EPSILON 1e-6f

static float cstar_sampling_dist_point_segment(point_t p, point_t a, point_t b)
{
    float vx = b.x - a.x;
    float vy = b.y - a.y;
    float wx = p.x - a.x;
    float wy = p.y - a.y;

    float vv = vx * vx + vy * vy;
    if (vv <= CSTAR_EPSILON)
    {
        float dx = p.x - a.x;
        float dy = p.y - a.y;
        return sqrtf(dx * dx + dy * dy);
    }

    float t = (wx * vx + wy * vy) / vv;
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    float cx = a.x + t * vx;
    float cy = a.y + t * vy;
    float dx = p.x - cx;
    float dy = p.y - cy;
    return sqrtf(dx * dx + dy * dy);
}

static bool cstar_sampling_point_in_polygon(point_t point, const polygon_t *polygon)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 3u)
    {
        return false;
    }

    bool inside = false;
    uint32_t j = polygon->vertex_count - 1u;
    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        point_t vi = polygon->vertices[i];
        point_t vj = polygon->vertices[j];

        bool intersects = ((vi.y > point.y) != (vj.y > point.y)) &&
                          (point.x < (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + CSTAR_EPSILON) + vi.x);
        if (intersects)
        {
            inside = !inside;
        }

        j = i;
    }

    return inside;
}

static bool cstar_sampling_point_on_boundary(point_t point, const polygon_t *polygon, float tol)
{
    if (polygon == NULL || polygon->vertices == NULL || polygon->vertex_count < 2u)
    {
        return false;
    }

    for (uint32_t i = 0; i < polygon->vertex_count; ++i)
    {
        uint32_t next = (i + 1u) % polygon->vertex_count;
        point_t a = polygon->vertices[i];
        point_t b = polygon->vertices[next];

        if (cstar_sampling_dist_point_segment(point, a, b) <= tol)
        {
            return true;
        }
    }

    return false;
}

static bool cstar_sampling_point_in_any_obstacle(point_t point, const cstar_environment_t *env)
{
    if (env == NULL || env->operationalObstacles == NULL || env->obstacle_count == 0u)
    {
        return false;
    }

    for (uint32_t obstacle_index = 0; obstacle_index < env->obstacle_count; ++obstacle_index)
    {
        const polygon_t *obstacle = &env->operationalObstacles[obstacle_index];
        if (cstar_sampling_point_in_polygon(point, obstacle) ||
            cstar_sampling_point_on_boundary(point, obstacle, CSTAR_EPSILON))
        {
            return true;
        }
    }

    return false;
}

static bool cstar_sampling_point_on_any_boundary(point_t point, const cstar_environment_t *env, float tol)
{
    if (env == NULL)
    {
        return false;
    }

    if (cstar_sampling_point_on_boundary(point, &env->operationalBoundary, tol))
    {
        return true;
    }

    if (env->operationalObstacles == NULL || env->obstacle_count == 0u)
    {
        return false;
    }

    for (uint32_t obstacle_index = 0; obstacle_index < env->obstacle_count; ++obstacle_index)
    {
        if (cstar_sampling_point_on_boundary(point, &env->operationalObstacles[obstacle_index], tol))
        {
            return true;
        }
    }

    return false;
}

static bool cstar_sampling_point_is_free(point_t point, const cstar_environment_t *env)
{
    if (env == NULL)
    {
        return false;
    }

    if (!cstar_sampling_point_in_polygon(point, &env->operationalBoundary) &&
        !cstar_sampling_point_on_boundary(point, &env->operationalBoundary, CSTAR_EPSILON))
    {
        return false;
    }

    if (cstar_sampling_point_in_any_obstacle(point, env))
    {
        return false;
    }

    return true;
}

static bool cstar_sampling_is_end_node_vertical(point_t sample, float w, const cstar_environment_t *env)
{
    if (env == NULL || w <= CSTAR_EPSILON)
    {
        return false;
    }

    point_t probe_up = {sample.x, sample.y + w};
    point_t probe_down = {sample.x, sample.y - w};

    bool up_hits_boundary = cstar_sampling_point_on_any_boundary(probe_up, env, CSTAR_EPSILON);
    bool down_hits_boundary = cstar_sampling_point_on_any_boundary(probe_down, env, CSTAR_EPSILON);
    bool up_blocked = !cstar_sampling_point_is_free(probe_up, env);
    bool down_blocked = !cstar_sampling_point_is_free(probe_down, env);

    return up_hits_boundary || down_hits_boundary || up_blocked || down_blocked;
}

static bool cstar_sampling_is_surrounded_node_vertical(point_t sample, float w, const cstar_environment_t *env)
{
    if (env == NULL || w <= CSTAR_EPSILON)
    {
        return false;
    }

    point_t probe_up = {sample.x, sample.y + w};
    point_t probe_down = {sample.x, sample.y - w};

    bool up_blocked = !cstar_sampling_point_is_free(probe_up, env) || cstar_sampling_point_on_any_boundary(probe_up, env, CSTAR_EPSILON);
    bool down_blocked = !cstar_sampling_point_is_free(probe_down, env) || cstar_sampling_point_on_any_boundary(probe_down, env, CSTAR_EPSILON);

    return up_blocked && down_blocked;
}

static float cstar_sampling_dist_to_operational_boundary(point_t point, const cstar_environment_t *env)
{
    if (env == NULL ||
        env->operationalBoundary.vertices == NULL ||
        env->operationalBoundary.vertex_count < 2u)
    {
        return INFINITY;
    }

    float min_dist = INFINITY;
    const polygon_t *boundary = &env->operationalBoundary;

    for (uint32_t i = 0; i < boundary->vertex_count; ++i)
    {
        uint32_t next = (i + 1u) % boundary->vertex_count;
        float dist = cstar_sampling_dist_point_segment(point,
                                                       boundary->vertices[i],
                                                       boundary->vertices[next]);
        if (dist < min_dist)
        {
            min_dist = dist;
        }
    }

    return min_dist;
}

int cstar_generate_frontier_samples(cstar_rcg_t *rcg,
                                    float w,
                                    int delta,
                                    const cstar_environment_t *env)
{
    if (env == NULL || env->laps == NULL || rcg == NULL)
    {
        return 0;
    }

    cstar_lap_t *laps = (cstar_lap_t *)env->laps;
    int lap_count = (int)cvector_size(laps);
    if (lap_count <= 0)
    {
        return 0;
    }

    float min_x = 0.0f, max_x = 0.0f, min_y = 0.0f, max_y = 0.0f;
    cstar_lap_boundary_bbox(env, &min_x, &max_x, &min_y, &max_y);

    float step = ((delta > 0 ? (float)delta : 1.0f) * ((w > CSTAR_EPSILON) ? w : 1.0f));
    float anchor_y = env->start_point.y;
    int first_sample_index = (int)ceilf(((min_y - anchor_y) / step) - CSTAR_EPSILON);
    int last_sample_index = (int)floorf(((max_y - anchor_y) / step) + CSTAR_EPSILON);

    int start_lap_index = 0;
    float closest_lap_dx = fabsf(laps[0].x - env->start_point.x);
    for (int i = 1; i < lap_count; ++i)
    {
        float dx = fabsf(laps[i].x - env->start_point.x);
        if (dx < closest_lap_dx)
        {
            closest_lap_dx = dx;
            start_lap_index = i;
        }
    }

    int total_added = 0;

    for (int lap_index = 0; lap_index < lap_count; ++lap_index)
    {
        cstar_lap_t *lap = &laps[lap_index];

        if (lap->node_ids != NULL)
        {
            cvector_free(lap->node_ids);
            lap->node_ids = NULL;
            lap->node_count = 0;
            lap->node_capacity = 0;
        }

        /* Force first sample at start point on nearest start lap. */
        bool forced_start_sample_added = false;
        if (lap_index == start_lap_index)
        {
            point_t start_sample = {env->start_point.x, env->start_point.y};
            if (cstar_sampling_point_is_free(start_sample, env))
            {
                bool is_end_node = cstar_sampling_is_end_node_vertical(start_sample, w, env);
                bool is_surrounded_node = cstar_sampling_is_surrounded_node_vertical(start_sample, w, env);
                int start_node_id = cstar_rcg_add_node(rcg, start_sample, lap->id, is_end_node, is_surrounded_node, true);
                if (start_node_id != CSTAR_NO_NEIGHBOR)
                {
                    cvector_push_back(lap->node_ids, start_node_id);
                    forced_start_sample_added = true;
                    total_added++;
                }
            }
        }

        for (int sample_index = first_sample_index; sample_index <= last_sample_index; ++sample_index)
        {
            float y = anchor_y + ((float)sample_index * step);

            if (forced_start_sample_added && fabsf(y - env->start_point.y) <= CSTAR_EPSILON)
            {
                continue;
            }

            point_t sample = {lap->x, y};

            if (!cstar_sampling_point_is_free(sample, env))
            {
                continue;
            }

            float max_boundary_distance = w;
            float boundary_distance = cstar_sampling_dist_to_operational_boundary(sample, env);
            if (!(boundary_distance < max_boundary_distance))
            {
                continue;
            }

            bool is_end_node = cstar_sampling_is_end_node_vertical(sample, w, env);
            bool is_surrounded_node = cstar_sampling_is_surrounded_node_vertical(sample, w, env);
            int node_id = cstar_rcg_add_node(rcg, sample, lap->id, is_end_node, is_surrounded_node, false);
            if (node_id == CSTAR_NO_NEIGHBOR)
            {
                continue;
            }

            cvector_push_back(lap->node_ids, node_id);
            total_added++;
        }

        lap->node_count = (int)cvector_size(lap->node_ids);
        lap->node_capacity = cvector_capacity(lap->node_ids);
    }

    return total_added;
}
